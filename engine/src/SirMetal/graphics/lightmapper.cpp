#include "SirMetal/graphics/lightmapper.h"
#include "SirMetal/engine.h"
#include "SirMetal/graphics/renderingContext.h"
#include "SirMetal/resources/gltfLoader.h"
#include "SirMetal/resources/meshes/meshManager.h"
#include "SirMetal/resources/shaderManager.h"
#include "SirMetal/resources/textureManager.h"
#include "rectpack2D/src/finders_interface.h"
#include <Metal/Metal.h>
#include "SirMetal/graphics/PSOGenerator.h"
#include "SirMetal/graphics/materialManager.h"

namespace SirMetal::graphics {

void LightMapper::initialize(EngineContext *context, const char *gbufferShader,
                             const char *rtShader) {
  m_gbuffHandle = context->m_shaderManager->loadShader(gbufferShader);
  m_rtLightMapHandle = context->m_shaderManager->loadShader(rtShader);
}
void LightMapper::setAssetData(EngineContext *context, GLTFAsset *asset,
                               int individualLightMapSize) {
  m_asset = asset;
  m_lightMapSize = individualLightMapSize;
  buildMultiLevelBVH(context, asset->models, accelStruct);
  packResult = buildPacking(16384, individualLightMapSize, asset->models.size());
  allocateTextures(context, packResult.w, packResult.h);
  recordRtArgBuffer(context, asset);
  recordRasterArgBuffer(context, asset);
}

void LightMapper::recordRtArgBuffer(EngineContext *context, GLTFAsset *asset) {
  id<MTLDevice> device = context->m_renderingContext->getDevice();

  // args buffer
  id<MTLFunction> fn = context->m_shaderManager->getKernelFunction(m_rtLightMapHandle);
  id<MTLArgumentEncoder> argumentEncoder = [fn newArgumentEncoderWithBufferIndex:2];

  int meshesCount = asset->models.size();
  int buffInstanceSize = argumentEncoder.encodedLength;
  argRtBuffer = [device newBufferWithLength:buffInstanceSize * meshesCount options:0];

  for (int i = 0; i < meshesCount; ++i) {
    [argumentEncoder setArgumentBuffer:argRtBuffer offset:i * buffInstanceSize];
    const auto *meshData = context->m_meshManager->getMeshData(asset->models[i].mesh);
    [argumentEncoder setBuffer:meshData->vertexBuffer
                        offset:meshData->ranges[0].m_offset
                       atIndex:0];
    [argumentEncoder setBuffer:meshData->vertexBuffer
                        offset:meshData->ranges[1].m_offset
                       atIndex:1];
    [argumentEncoder setBuffer:meshData->vertexBuffer
                        offset:meshData->ranges[2].m_offset
                       atIndex:2];
    [argumentEncoder setBuffer:meshData->vertexBuffer
                        offset:meshData->ranges[3].m_offset
                       atIndex:3];
    [argumentEncoder setBuffer:meshData->indexBuffer offset:0 atIndex:4];
    const auto &material = asset->materials[i];
    id albedo = context->m_textureManager->getNativeFromHandle(material.colorTexture);
    [argumentEncoder setTexture:albedo atIndex:5];

    auto *ptrMatrix = [argumentEncoder constantDataAtIndex:6];
    memcpy(ptrMatrix, &asset->models[i].matrix, sizeof(float) * 16);
    memcpy(((char *) ptrMatrix + sizeof(float) * 16), &material.colorFactors,
           sizeof(float) * 4);
  }
}
void LightMapper::allocateTextures(EngineContext *context, int w, int h) {

  id<MTLDevice> device = context->m_renderingContext->getDevice();
  SirMetal::AllocTextureRequest request{static_cast<uint32_t>(w),
                                        static_cast<uint32_t>(h),
                                        1,
                                        MTLTextureType2D,
                                        MTLPixelFormatRGBA32Float,
                                        MTLTextureUsageRenderTarget |
                                                MTLTextureUsageShaderRead,
                                        MTLStorageModePrivate,
                                        1,
                                        "gbuffPositions"};
  m_gbuff[0] = context->m_textureManager->allocate(device, request);
  auto oldUsage = request.usage;
  request.usage = oldUsage | MTLTextureUsageShaderWrite;
  request.name = "lightMap";
  m_lightMap = context->m_textureManager->allocate(device, request);
  request.usage = oldUsage;
  request.format = MTLPixelFormatRG16Unorm;
  request.name = "gbuffUVs";
  m_gbuff[1] = context->m_textureManager->allocate(device, request);
  request.format = MTLPixelFormatRGBA8Unorm;
  request.name = "gbuffNormals";
  m_gbuff[2] = context->m_textureManager->allocate(device, request);
}

PackingResult LightMapper::buildPacking(int maxSize, int individualSize, int count) {
  const auto runtime_flipping_mode = rectpack2D::flipping_option::DISABLED;

  constexpr bool allow_flip = false;
  using spaces_type =
          rectpack2D::empty_spaces<allow_flip, rectpack2D::default_empty_spaces>;

  using rect_type = rectpack2D::output_rect_t<spaces_type>;

  auto report_successful = [](rect_type &) {
    return rectpack2D::callback_result::CONTINUE_PACKING;
  };

  auto report_unsuccessful = [](rect_type &) {
    return rectpack2D::callback_result::ABORT_PACKING;
  };


  const auto discard_step = -4;

  std::vector<rect_type> rectangles(count);
  for (int i = 0; i < count; ++i) {
    rectangles[i] = rectpack2D::rect_xywh(0, 0, individualSize, individualSize);
  }

  auto report_result = [&rectangles](const rectpack2D::rect_wh &result_size) {
    printf("Resultant bin: %i %i \n", result_size.w, result_size.h);

    for (const auto &r : rectangles) { printf("%i %i %i %i\n", r.x, r.y, r.w, r.h); }
  };

  {
    //TODO investigate if is always the case
    //this way should respect the rectangles order so we don't lose track of which one is what
    const auto result_size = rectpack2D::find_best_packing_dont_sort<spaces_type>(
            rectangles, make_finder_input(maxSize, discard_step, report_successful,
                                          report_unsuccessful, runtime_flipping_mode));
    report_result(result_size);
    std::vector<TexRect> outRects;
    for (const auto &rect : rectangles) {
      outRects.emplace_back(TexRect{rect.x, rect.y, rect.w, rect.h});
    }
    return {
            outRects,
            result_size.w,
            result_size.h,
    };
  }
}
void LightMapper::doGBufferPass(EngineContext* context, id<MTLCommandBuffer> commandBuffer) {

  SirMetal::graphics::DrawTracker tracker{};
  id g1 = context->m_textureManager->getNativeFromHandle(m_gbuff[0]);
  id g2 = context->m_textureManager->getNativeFromHandle(m_gbuff[1]);
  id g3 = context->m_textureManager->getNativeFromHandle(m_gbuff[2]);
  tracker.renderTargets[0] = g1;
  tracker.renderTargets[1] = g2;
  tracker.renderTargets[2] = g3;
  tracker.depthTarget = {};

  PSOCache cache =
          SirMetal::getPSO(context, tracker, SirMetal::Material{"gbuff", false});

  // blitting to the swap chain
  MTLRenderPassDescriptor *passDescriptor =
          [MTLRenderPassDescriptor renderPassDescriptor];
  passDescriptor.colorAttachments[0].texture = g1;
  passDescriptor.colorAttachments[0].clearColor = {0.0, 0.0, 0.0, 0.0};
  passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
  passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;

  passDescriptor.colorAttachments[1].texture = g2;
  passDescriptor.colorAttachments[1].clearColor = {0.0, 0.0, 0.0, 0.0};
  passDescriptor.colorAttachments[1].storeAction = MTLStoreActionStore;
  passDescriptor.colorAttachments[1].loadAction = MTLLoadActionClear;

  passDescriptor.colorAttachments[2].texture = g3;
  passDescriptor.colorAttachments[2].clearColor = {0.0, 0.0, 0.0, 0.0};
  passDescriptor.colorAttachments[2].storeAction = MTLStoreActionStore;
  passDescriptor.colorAttachments[2].loadAction = MTLLoadActionClear;

  id<MTLRenderCommandEncoder> commandEncoder =
          [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];

  [commandEncoder setRenderPipelineState:cache.color];
  [commandEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
  //disabling culling since we  are rendering in uv space
  [commandEncoder setCullMode:MTLCullModeNone];

  //setting the arguments buffer
  [commandEncoder setVertexBuffer:argBuffer offset:0 atIndex:0];
  [commandEncoder setFragmentBuffer:argBufferFrag offset:0 atIndex:0];

  //compute jitter
  double LO = 0.0f;
  double HI = 1.0f;
  float jitter[2];
  auto t1 = static_cast<float>(LO + static_cast<double>(rand()) /
                                            (static_cast<double>(RAND_MAX / (HI - LO))) *
                                            M_PI * 2);
  auto t2 = static_cast<float>(LO + static_cast<double>(rand()) /
                                            (static_cast<double>(RAND_MAX / (HI - LO))) *
                                            M_PI * 2);

  float jitterMultiplier = 3.0f;
  jitter[0] = cos(t1) / m_lightMapSize;
  jitter[1] = sin(t2) / m_lightMapSize;
  jitter[0] *= (jitterMultiplier);
  jitter[1] *= (jitterMultiplier);

  for (int i = 0; i < m_asset->models.size(); ++i) {
    const auto &mesh = m_asset->models[i];
    const SirMetal::MeshData *meshData = context->m_meshManager->getMeshData(mesh.mesh);
    auto *mat = (void *) (&mesh.matrix);
    const auto &packrect = packResult.rectangles[i];
    MTLScissorRect rect;
    rect.width = packrect.w;
    rect.height = packrect.h;
    rect.x = packrect.x;
    rect.y = packrect.y;
    [commandEncoder setScissorRect:rect];

    MTLViewport view;
    view.width = rect.width;
    view.height = rect.height;
    view.originX = rect.x;
    view.originY = rect.y;
    view.znear = 0;
    view.zfar = 1;
    [commandEncoder setViewport:view];
    [commandEncoder useResource:meshData->vertexBuffer usage:MTLResourceUsageRead];
    [commandEncoder useResource:context->m_textureManager->getNativeFromHandle(
                                        m_asset->materials[i].colorTexture)
                          usage:MTLResourceUsageSample];
    [commandEncoder setVertexBytes:mat length:16 * sizeof(float) atIndex:5];
    [commandEncoder setVertexBytes:&i length:sizeof(uint32_t) atIndex:6];
    [commandEncoder setVertexBytes:&jitter[0] length:sizeof(float) * 2 atIndex:7];
    [commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                               indexCount:meshData->primitivesCount
                                indexType:MTLIndexTypeUInt32
                              indexBuffer:meshData->indexBuffer
                        indexBufferOffset:0];
    rect.x = 0;
    rect.y = 0;
    rect.width = context->m_config.m_windowConfig.m_width;
    rect.height = context->m_config.m_windowConfig.m_height;
    [commandEncoder setScissorRect:rect];
    view.height = rect.height;
    view.width = rect.width;
    view.originX = 0;
    view.originY = 0;
    view.znear = 0;
    view.zfar = 1;
    [commandEncoder setViewport:view];
  }
  [commandEncoder endEncoding];
}
void LightMapper::recordRasterArgBuffer(EngineContext*context, GLTFAsset* asset) {
  id<MTLDevice> device = context->m_renderingContext->getDevice();
  // args buffer
  id<MTLFunction> fn = context->m_shaderManager->getVertexFunction(m_gbuffHandle);
  id<MTLArgumentEncoder> argumentEncoder = [fn newArgumentEncoderWithBufferIndex:0];

  id<MTLFunction> fnFrag = context->m_shaderManager->getFragmentFunction(m_gbuffHandle);
  id<MTLArgumentEncoder> argumentEncoderFrag =
  [fnFrag newArgumentEncoderWithBufferIndex:0];

  int meshesCount = asset->models.size();
  int buffInstanceSize = argumentEncoder.encodedLength;
  argBuffer = [device newBufferWithLength:buffInstanceSize * meshesCount options:0];

  int buffInstanceSizeFrag = argumentEncoderFrag.encodedLength;
  argBufferFrag =
  [device newBufferWithLength:buffInstanceSizeFrag * meshesCount options:0];


  for (int i = 0; i < meshesCount; ++i) {
    [argumentEncoder setArgumentBuffer:argBuffer offset:i * buffInstanceSize];
    const auto *meshData = context->m_meshManager->getMeshData(asset->models[i].mesh);
    [argumentEncoder setBuffer:meshData->vertexBuffer
    offset:meshData->ranges[0].m_offset
    atIndex:0];
    [argumentEncoder setBuffer:meshData->vertexBuffer
    offset:meshData->ranges[1].m_offset
    atIndex:1];
    [argumentEncoder setBuffer:meshData->vertexBuffer
    offset:meshData->ranges[2].m_offset
    atIndex:2];
    [argumentEncoder setBuffer:meshData->vertexBuffer
    offset:meshData->ranges[3].m_offset
    atIndex:3];
    [argumentEncoder setBuffer:meshData->vertexBuffer
    offset:meshData->ranges[4].m_offset
    atIndex:4];

    const auto &material = asset->materials[i];
    [argumentEncoderFrag setArgumentBuffer:argBufferFrag offset:i * buffInstanceSizeFrag];

    id albedo;
    albedo = context->m_textureManager->getNativeFromHandle(m_lightMap);
    //}
    [argumentEncoderFrag setTexture:albedo atIndex:0];
    auto *ptr = [argumentEncoderFrag constantDataAtIndex:1];
    float matData[8]{};
    matData[0] = material.colorFactors.x;
    matData[1] = material.colorFactors.y;
    matData[2] = material.colorFactors.z;
    matData[3] = material.colorFactors.w;

    //uv offset for lightmap

    float wratio = static_cast<float>(packResult.rectangles[i].w) /
                   static_cast<float>(packResult.w);
    float hratio = static_cast<float>(packResult.rectangles[i].h) /
                   static_cast<float>(packResult.h);
    float xoff = static_cast<float>(packResult.rectangles[i].x) /
                 static_cast<float>(packResult.w);
    float yoff = static_cast<float>(packResult.rectangles[i].y) /
                 static_cast<float>(packResult.h);

    matData[4] = wratio;
    matData[5] = hratio;
    matData[6] = xoff;
    matData[7] = yoff;

    memcpy(ptr, &matData, sizeof(float) * 8);
  }


}
}// namespace SirMetal::graphics