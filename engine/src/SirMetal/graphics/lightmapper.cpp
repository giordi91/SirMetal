#include "SirMetal/graphics/lightmapper.h"
#include "SirMetal/engine.h"
#include "SirMetal/graphics/PSOGenerator.h"
#include "SirMetal/graphics/constantBufferManager.h"
#include "SirMetal/graphics/materialManager.h"
#include "SirMetal/graphics/renderingContext.h"
#include "SirMetal/resources/gltfLoader.h"
#include "SirMetal/resources/meshes/meshManager.h"
#include "SirMetal/resources/shaderManager.h"
#include "SirMetal/resources/textureManager.h"
#include "rectpack2D/src/finders_interface.h"
#include <Metal/Metal.h>

namespace SirMetal::graphics {
static id createComputePipeline(id<MTLDevice> device, id function) {
  // Create compute pipelines will will execute code on the GPU
  MTLComputePipelineDescriptor *computeDescriptor =
          [[MTLComputePipelineDescriptor alloc] init];

  // Set to YES to allow compiler to make certain optimizations
  computeDescriptor.threadGroupSizeIsMultipleOfThreadExecutionWidth = YES;

  // Generates rays according to view/projection matrices
  computeDescriptor.computeFunction = function;
  NSError *error = nullptr;
  id toReturn = [device newComputePipelineStateWithDescriptor:computeDescriptor
                                                      options:0
                                                   reflection:nil
                                                        error:&error];

  if (!toReturn) NSLog(@"Failed to create pipeline state: %@", error);

  return toReturn;
}

void LightMapper::initialize(EngineContext *context, const char *gbufferShader,
                             const char *gbufferClearShader, const char *rtShader) {
  m_gbuffHandle = context->m_shaderManager->loadShader(gbufferShader);
  m_gbuffClearHandle = context->m_shaderManager->loadShader(gbufferClearShader);
  m_rtLightMapHandle = context->m_shaderManager->loadShader(rtShader);

  id<MTLDevice> device = context->m_renderingContext->getDevice();
  int rwtier = device.readWriteTextureSupport;
  assert(rwtier == 2 && "to read and write RGBA32Float in place RW texture tier 2 is required");
  m_rtLightmapPipeline = createComputePipeline(
          device, context->m_shaderManager->getKernelFunction(m_rtLightMapHandle));
}
void LightMapper::setAssetData(EngineContext *context, GLTFAsset *asset,
                               int individualLightMapSize) {
  m_asset = asset;
  m_lightMapSize = individualLightMapSize;
#if RT
  //we use a multi level bvh, as of now does not optimize for instances, for each model a separated
  //accel structure is built (the bottom level, from there each bottom level is used with a transform in the
  //top level accel structure
  buildMultiLevelBVH(context, asset->models, m_accelStruct);
#endif
  //here we build the packing result, we pass in the max texture, as of now there is no way to query
  //it, so we use an hardcoded value. all the ligthmaps are the same size, and we are going to have one
  //per model
  m_packResult = buildPacking(16384, individualLightMapSize, asset->models.size());
  //the packing told us how big the atlases needs to be, so we can allocate the necessary textures
  allocateTextures(context, m_packResult.w, m_packResult.h);

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
  m_argRtBuffer = [device newBufferWithLength:buffInstanceSize * meshesCount options:0];

  for (int i = 0; i < meshesCount; ++i) {
    [argumentEncoder setArgumentBuffer:m_argRtBuffer offset:i * buffInstanceSize];
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
                                        MTLPixelFormatR32Uint,
                                        MTLTextureUsageRenderTarget |
                                                MTLTextureUsageShaderRead,
                                        MTLStorageModePrivate,
                                        1,
                                        "gbuffPositions"};
  m_gbuff[0] = context->m_textureManager->allocate(device, request);
  auto oldUsage = request.usage;
  request.format = MTLPixelFormatRGBA32Float;
  request.usage = oldUsage | MTLTextureUsageShaderWrite;
  request.name = "lightMap";
  m_lightMap = context->m_textureManager->allocate(device, request);
  request.usage = oldUsage;
  request.format = MTLPixelFormatRG16Unorm;
  request.name = "gbuffUVs";
  m_gbuff[1] = context->m_textureManager->allocate(device, request);
}

PackingResult LightMapper::buildPacking(int maxSize, int individualSize, int count) {
  //this is based on the sample of rectpack2D from:
  //https://github.com/TeamHypersomnia/rectpack2D
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
void LightMapper::doGBufferPass(EngineContext *context,
                                id<MTLCommandBuffer> commandBuffer) {

  //as of now, metal raytracing API do not allow to shoot rays outside the compute shader
  //stage, this forces us to use a gbuffer pass to store required information.
  //The lightmapper works in texture space, this means that we outpout the lightmap uvs
  //as a position to effectively flatten out the mesh in NDC coordinates. we will use that
  //information to get a starting point for the raytrace
  SirMetal::graphics::DrawTracker tracker{};
  id g1 = context->m_textureManager->getNativeFromHandle(m_gbuff[0]);
  id g2 = context->m_textureManager->getNativeFromHandle(m_gbuff[1]);
  //we only use two textures, one texture (uint32) records the primitive id of the mesh
  //the texture (RG32)stores the barycentric coordinates. This data is enough to reconstruct
  //the full fragment data
  tracker.renderTargets[0] = g1;
  tracker.renderTargets[1] = g2;
  tracker.depthTarget = {};


  MTLRenderPassDescriptor *passDescriptor =
          [MTLRenderPassDescriptor renderPassDescriptor];
  passDescriptor.colorAttachments[0].texture = g1;
  //To note, it is critical that we do not shoot rays from where there is no mesh written in the gbuffer pass
  //that is not particularly an issue per se, but is an issue because we do jitter the gbuffer to get around
  //missing conservative rasterization. I did not find a way to clear the buffer to the value 0xFFFFFFFF
  //so we will use a different shader to do the clear and leave the pass clear as don't care
  passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
  passDescriptor.colorAttachments[0].loadAction = MTLLoadActionDontCare;

  passDescriptor.colorAttachments[1].texture = g2;
  passDescriptor.colorAttachments[1].storeAction = MTLStoreActionStore;
  passDescriptor.colorAttachments[1].loadAction = MTLLoadActionDontCare;

  id<MTLRenderCommandEncoder> commandEncoder =
          [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];

  //we start by clearning the gbuffers to the values we want
  PSOCache cache =
          SirMetal::getPSO(context, tracker, SirMetal::Material{"gbuffClear", false});

  [commandEncoder setRenderPipelineState:cache.color];
  [commandEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
  //disabling culling since we  are rendering in uv space
  [commandEncoder setCullMode:MTLCullModeNone];

  //setting the arguments buffer
  [commandEncoder setVertexBuffer:m_argBuffer offset:0 atIndex:0];

  //cleaning the buffer
  [commandEncoder setRenderPipelineState:cache.color];
  [commandEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];

  //now we can perform the actual gbuffer pass
  cache = SirMetal::getPSO(context, tracker, SirMetal::Material{"gbuff", false});
  [commandEncoder setRenderPipelineState:cache.color];


  //the gbuffer is jittered to get around the missing conservative raster feature in metal.
  //we compute a random jitter value per frame normalize with the size of the lightmap
  double LO = 0.0f;
  double HI = 1.0f;
  float jitter[2];
  auto t1 = static_cast<float>(LO + static_cast<double>(rand()) /
                                            (static_cast<double>(RAND_MAX / (HI - LO))) *
                                            M_PI * 2);
  auto t2 = static_cast<float>(LO + static_cast<double>(rand()) /
                                            (static_cast<double>(RAND_MAX / (HI - LO))) *
                                            M_PI * 2);

  float jitterMultiplier = 5.0f;
  jitter[0] = cos(t1) / m_lightMapSize;
  jitter[1] = sin(t2) / m_lightMapSize;
  jitter[0] *= (jitterMultiplier);
  jitter[1] *= (jitterMultiplier);

  MTLScissorRect rect;
  MTLViewport view;
  //rasterizing in the gbuffer each mesh
  for (int i = 0; i < m_asset->models.size(); ++i) {
    const auto &mesh = m_asset->models[i];
    const SirMetal::MeshData *meshData = context->m_meshManager->getMeshData(mesh.mesh);
    auto *mat = (void *) (&mesh.matrix);
    //we use the packing result to figure out where in the atlas we need to render
    const auto &packrect = m_packResult.rectangles[i];
    rect.width = packrect.w;
    rect.height = packrect.h;
    rect.x = packrect.x;
    rect.y = packrect.y;
    [commandEncoder setScissorRect:rect];

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
  }
  //when we are done we reset the view and scissor to default
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
  [commandEncoder endEncoding];
}
void LightMapper::recordRasterArgBuffer(EngineContext *context, GLTFAsset *asset) {
  id<MTLDevice> device = context->m_renderingContext->getDevice();
  // args buffer
  id<MTLFunction> fn = context->m_shaderManager->getVertexFunction(m_gbuffHandle);
  id<MTLArgumentEncoder> argumentEncoder = [fn newArgumentEncoderWithBufferIndex:0];

  int meshesCount = asset->models.size();
  int buffInstanceSize = argumentEncoder.encodedLength;
  m_argBuffer = [device newBufferWithLength:buffInstanceSize * meshesCount options:0];


  for (int i = 0; i < meshesCount; ++i) {
    [argumentEncoder setArgumentBuffer:m_argBuffer offset:i * buffInstanceSize];
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

  }
}
void LightMapper::bakeNextSample(EngineContext *context,
                                 id<MTLCommandBuffer> commandBuffer,
                                 ConstantBufferHandle uniforms, id randomTexture) {

  doGBufferPass(context, commandBuffer);
#if RT
  doLightMapBake(context, commandBuffer, uniforms, randomTexture);
#endif
}
void LightMapper::doLightMapBake(EngineContext *context,
                                 id<MTLCommandBuffer> commandBuffer,
                                 ConstantBufferHandle uniforms, id randomTexture) {
  //the lightmap bake is fairly simple on the cpu side, simply dispatch the compute shader
  //performing the RT.
  //To avoid overwhelming the frame and keep interactivity, we do one section of the gbuffer per frame
  int index = context->m_timings.m_totalNumberOfFrames % m_asset->models.size();

  int w = m_lightMapSize;
  int h = m_lightMapSize;

  MTLSize threadsPerThreadgroup = MTLSizeMake(8, 8, 1);
  MTLSize threadgroups = MTLSizeMake(
          (w + threadsPerThreadgroup.width - 1) / threadsPerThreadgroup.width,
          (h + threadsPerThreadgroup.height - 1) / threadsPerThreadgroup.height, 1);

  id<MTLComputeCommandEncoder> computeEncoder = [commandBuffer computeCommandEncoder];

  auto bindInfo = context->m_constantBufferManager->getBindInfo(context, uniforms);
  id<MTLTexture> colorTexture =
          context->m_textureManager->getNativeFromHandle(m_lightMap);

  id g1 = context->m_textureManager->getNativeFromHandle(m_gbuff[0]);
  id g2 = context->m_textureManager->getNativeFromHandle(m_gbuff[1]);

  [computeEncoder setAccelerationStructure:m_accelStruct.instanceAccelerationStructure
                             atBufferIndex:0];
  [computeEncoder setBuffer:bindInfo.buffer offset:bindInfo.offset atIndex:1];
  [computeEncoder setBuffer:m_argRtBuffer offset:0 atIndex:2];
  [computeEncoder setBytes:&index length:4 atIndex:3];
  int off[] = {m_packResult.rectangles[index].x, m_packResult.rectangles[index].y};
  [computeEncoder setBytes:&off[0] length:sizeof(int) * 2 atIndex:4];
  [computeEncoder setTexture:colorTexture atIndex:0];
  [computeEncoder setTexture:randomTexture atIndex:1];
  [computeEncoder setTexture:g1 atIndex:3];
  [computeEncoder setTexture:g2 atIndex:4];
  [computeEncoder setComputePipelineState:m_rtLightmapPipeline];
  [computeEncoder dispatchThreadgroups:threadgroups
                 threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];
  [computeEncoder endEncoding];

  //we only increment the sample count when we performed a full pass of the whole atlas
  ++m_rtFrameCounterFull;
  if ((m_rtFrameCounterFull % m_asset->models.size()) == 0) { m_rtSampleCounter++; }
}
}// namespace SirMetal::graphics