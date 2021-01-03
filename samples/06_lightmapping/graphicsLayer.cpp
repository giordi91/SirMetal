#include "graphicsLayer.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <SirMetal/core/mathUtils.h>

#include "SirMetal/application/window.h"
#include "SirMetal/core/input.h"
#include "SirMetal/engine.h"
#include "SirMetal/graphics/blit.h"
#include "SirMetal/graphics/constantBufferManager.h"
#include "SirMetal/graphics/debug/debugRenderer.h"
#include "SirMetal/graphics/debug/imgui/imgui.h"
#include "SirMetal/graphics/debug/imguiRenderer.h"
#include "SirMetal/graphics/materialManager.h"
#include "SirMetal/resources/meshes/meshManager.h"
#include "SirMetal/resources/shaderManager.h"
#include <SirMetal/resources/textureManager.h>

static const char *rts[] = {"GPositions", "GUVs","lightMap"};

struct RtCamera {
  simd_float4x4 VPinverse;
  vector_float3 position;
  vector_float3 right;
  vector_float3 up;
  vector_float3 forward;
};


struct Uniforms {
  unsigned int width;
  unsigned int height;
  unsigned int frameIndex;
  unsigned int lightMapSize;
  RtCamera camera;
};

constexpr int kMaxInflightBuffers = 3;

id createComputePipeline(id<MTLDevice> device, id function) {
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

namespace Sandbox {
void GraphicsLayer::onAttach(SirMetal::EngineContext *context) {

  m_engine = context;

  // initializing the camera to the identity
  m_camera.viewMatrix = matrix_float4x4_translation(vector_float3{0, 0, 0});
  m_camera.fov = M_PI / 4;
  m_camera.nearPlane = 0.01;
  m_camera.farPlane = 60;
  m_cameraController.setCamera(&m_camera);
  m_cameraController.setPosition(0, 1, 12);
  m_camConfig = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.2, 0.008};

  m_camUniformHandle = m_engine->m_constantBufferManager->allocate(
          m_engine, sizeof(SirMetal::Camera),
          SirMetal::CONSTANT_BUFFER_FLAGS_BITS::CONSTANT_BUFFER_FLAG_BUFFERED);
  m_uniforms = m_engine->m_constantBufferManager->allocate(
          m_engine, sizeof(Uniforms),
          SirMetal::CONSTANT_BUFFER_FLAGS_BITS::CONSTANT_BUFFER_FLAG_BUFFERED);

  const std::string base = m_engine->m_config.m_dataSourcePath + "/sandbox";
  // let us load the gltf file
  struct SirMetal::GLTFLoadOptions options;
  options.flags = SirMetal::GLTFLoadFlags::GLTF_LOAD_FLAGS_FLATTEN_HIERARCHY |
                  SirMetal::GLTF_LOAD_FLAGS_GENERATE_LIGHT_MAP_UVS;
  options.lightMapSize = lightMapSize;

  SirMetal::loadGLTF(m_engine, (base + +"/test.glb").c_str(), m_asset, options);


  id<MTLDevice> device = m_engine->m_renderingContext->getDevice();

  assert(device.supportsRaytracing == true &&
         "This device does not support raytracing API, you can try samples based on MPS");


  m_shaderHandle =
          m_engine->m_shaderManager->loadShader((base + "/Shaders.metal").c_str());
  m_fullScreenHandle =
          m_engine->m_shaderManager->loadShader((base + "/fullscreen.metal").c_str());

  m_lightMapper.initialize(m_engine, (base + "/gbuff.metal").c_str(),(base + "/gbuffClear.metal").c_str(),
                           (base + "/rtLightMap.metal").c_str());

  m_lightMapper.setAssetData(context, &m_asset, lightMapSize);

  recordRasterArgBuffer();

  SirMetal::AllocTextureRequest requestDepth{m_engine->m_config.m_windowConfig.m_width,
                                             m_engine->m_config.m_windowConfig.m_height,
                                             1,
                                             MTLTextureType2D,
                                             MTLPixelFormatDepth32Float_Stencil8,
                                             MTLTextureUsageRenderTarget |
                                                     MTLTextureUsageShaderRead,
                                             MTLStorageModePrivate,
                                             1,
                                             "depthTexture"};
  m_depthHandle = m_engine->m_textureManager->allocate(device, requestDepth);


  SirMetal::graphics::initImgui(m_engine);
  frameBoundarySemaphore = dispatch_semaphore_create(kMaxInflightBuffers);

  // this is to flush the gpu, should figure out why is not actually flushing
  // properly
  m_engine->m_renderingContext->flush();

  generateRandomTexture(lightMapSize, lightMapSize);

  MTLArgumentBuffersTier tier = [device argumentBuffersSupport];
  assert(tier == MTLArgumentBuffersTier2);
}

void GraphicsLayer::onDetach() {}

bool GraphicsLayer::updateUniformsForView(float screenWidth, float screenHeight,
                                          uint32_t lightMapSize) {

  SirMetal::Input *input = m_engine->m_inputManager;

  bool updated = false;
  if (!ImGui::GetIO().WantCaptureMouse) {
    updated = m_cameraController.update(m_camConfig, input);
  }

  m_cameraController.updateProjection(screenWidth, screenHeight);
  m_engine->m_constantBufferManager->update(m_engine, m_camUniformHandle, &m_camera);

  // update rt uniform
  Uniforms u{};

  u.camera.VPinverse = m_camera.VPInverse;
  simd_float4 pos = m_camera.viewMatrix.columns[3];
  u.camera.position = simd_float3{pos.x, pos.y, pos.z};
  simd_float4 f = simd_normalize(-m_camera.viewMatrix.columns[2]);
  u.camera.forward = simd_float3{f.x, f.y, f.z};
  simd_float4 up = simd_normalize(m_camera.viewMatrix.columns[1]);
  u.camera.up = simd_float3{up.x, up.y, up.z};
  simd_float4 right = simd_normalize(m_camera.viewMatrix.columns[0]);
  u.camera.right = simd_normalize(simd_float3{right.x, right.y, right.z});

  u.frameIndex = m_lightMapper.m_rtSampleCounter;
  u.height = m_engine->m_config.m_windowConfig.m_height;
  u.width = m_engine->m_config.m_windowConfig.m_width;
  u.lightMapSize = lightMapSize;
  // TODO add light data
  // u.light ...
  m_engine->m_constantBufferManager->update(m_engine, m_uniforms, &u);

  return updated;
}

void GraphicsLayer::onUpdate() {

  // NOTE: very temp ESC handy to close the game
  if (m_engine->m_inputManager->isKeyDown(SDL_SCANCODE_ESCAPE)) {
    SDL_Event sdlevent{};
    sdlevent.type = SDL_QUIT;
    SDL_PushEvent(&sdlevent);
  }

  CAMetalLayer *swapchain = m_engine->m_renderingContext->getSwapchain();
  id<MTLCommandQueue> queue = m_engine->m_renderingContext->getQueue();

  // waiting for resource
  dispatch_semaphore_wait(frameBoundarySemaphore, DISPATCH_TIME_FOREVER);

  id<CAMetalDrawable> surface = [swapchain nextDrawable];
  id<MTLTexture> texture = surface.texture;

  float w = texture.width;
  float h = texture.height;
  updateUniformsForView(w, h, lightMapSize);

  id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];

  if (m_lightMapper.m_rtSampleCounter < m_lightMapper.m_requestedSamples) {
    m_lightMapper.bakeNextSample(m_engine, commandBuffer, m_uniforms, m_randomTexture);
  }


  SirMetal::graphics::DrawTracker tracker{};
  tracker.renderTargets[0] = texture;
  tracker.depthTarget = m_engine->m_textureManager->getNativeFromHandle(m_depthHandle);

  SirMetal::PSOCache cache =
          SirMetal::getPSO(m_engine, tracker, SirMetal::Material{"Shaders", false});

  // blitting to the swap chain
  MTLRenderPassDescriptor *passDescriptor =
          [MTLRenderPassDescriptor renderPassDescriptor];
  passDescriptor.colorAttachments[0].texture = texture;
  passDescriptor.colorAttachments[0].clearColor = {0.2, 0.2, 0.2, 1.0};
  passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
  passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;

  MTLRenderPassDepthAttachmentDescriptor *depthAttachment =
          passDescriptor.depthAttachment;
  depthAttachment.texture =
          m_engine->m_textureManager->getNativeFromHandle(m_depthHandle);
  depthAttachment.clearDepth = 1.0;
  depthAttachment.storeAction = MTLStoreActionDontCare;
  depthAttachment.loadAction = MTLLoadActionClear;

  id<MTLRenderCommandEncoder> commandEncoder =
          [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];

  doRasterRender(commandEncoder, cache);

  if (debugFullScreen) {
    SirMetal::graphics::BlitRequest request{};
    switch (currentDebug) {
      case 0:
        request.srcTexture = m_lightMapper.m_gbuff[0];
        break;
      case 1:
        request.srcTexture = m_lightMapper.m_gbuff[1];
        break;
      case 2:
        request.srcTexture = m_lightMapper.m_lightMap;
        break;
    }
    request.dstTexture = texture;
    request.customView = 0;
    request.customScissor = 0;
    SirMetal::graphics::doBlit(m_engine, commandEncoder, request);
  }
  // render debug
  m_engine->m_debugRenderer->newFrame();
  float data[6]{0, 0, 0, 0, 100, 0};
  m_engine->m_debugRenderer->drawLines(data, sizeof(float) * 6,
                                       vector_float4{1, 0, 0, 1});
  m_engine->m_debugRenderer->render(m_engine, commandEncoder, tracker, m_camUniformHandle,
                                    300, 300);

  // ui
  SirMetal::graphics::imguiNewFrame(m_engine, passDescriptor);
  renderDebugWindow();
  SirMetal::graphics::imguiEndFrame(commandBuffer, commandEncoder);

  [commandEncoder endEncoding];

  [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer) {
    // GPU work is complete
    // Signal the semaphore to start the CPU work
    dispatch_semaphore_signal(frameBoundarySemaphore);
  }];
  [commandBuffer presentDrawable:surface];
  [commandBuffer commit];


  SDL_RenderPresent(m_engine->m_renderingContext->getRenderer());
  //}
}

bool GraphicsLayer::onEvent(SirMetal::Event &) {
  // TODO start processing events the game cares about, like
  // etc...
  return false;
}

void GraphicsLayer::clear() {
  m_engine->m_renderingContext->flush();
  SirMetal::graphics::shutdownImgui();
}
void GraphicsLayer::renderDebugWindow() {

  static bool p_open = false;
  if (m_engine->m_inputManager->isKeyReleased(SDL_SCANCODE_GRAVE)) { p_open = !p_open; }
  if (!p_open) { return; }
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
  if (ImGui::Begin("Debug", &p_open, 0)) {
    m_gpuInfo.render(m_engine->m_renderingContext->getDevice());
    m_timingsWidget.render(m_engine);

    //lets render the pathracer stuff
    if (ImGui::CollapsingHeader("LightMapper")) {

      ImGui::SliderInt("Number of samples", &m_lightMapper.m_requestedSamples, 0, 4000);
      ImGui::Separator();
      //bake button
      bool isDone = m_lightMapper.m_rtSampleCounter == m_lightMapper.m_requestedSamples;
      if (isDone) { ImGui::PushStyleColor(0, ImVec4(0, 1, 0, 1)); }
      if (ImGui::Button("Bake lightmap")) { m_lightMapper.m_rtSampleCounter = 0; }
      if (isDone) { ImGui::PopStyleColor(1); }

      ImGui::PushItemWidth(50.0f);
      ImGui::DragInt("Samples done", &m_lightMapper.m_rtSampleCounter, 0.0f);
      ImGui::PopItemWidth();
      ImGui::Separator();

      ImGui::Checkbox("Debug full screen", &debugFullScreen);
      if (debugFullScreen) { ImGui::Combo("Target", &currentDebug, rts, 3); }
    }
  }
  ImGui::End();
}

void GraphicsLayer::generateRandomTexture(uint32_t w, uint32_t h) {


  // Create a render target which the shading kernel can write to
  MTLTextureDescriptor *renderTargetDescriptor = [[MTLTextureDescriptor alloc] init];

  renderTargetDescriptor.textureType = MTLTextureType2D;
  renderTargetDescriptor.width = w;
  renderTargetDescriptor.height = h;

  // Stored in private memory because it will only be read and written from the
  // GPU
  renderTargetDescriptor.storageMode = MTLStorageModeManaged;

  renderTargetDescriptor.pixelFormat = MTLPixelFormatR32Uint;
  renderTargetDescriptor.usage = MTLTextureUsageShaderRead;

  // Generate a texture containing a random integer value for each pixel. This
  // value will be used to decorrelate pixels while drawing pseudorandom numbers
  // from the Halton sequence.
  m_randomTexture = [m_engine->m_renderingContext->getDevice()
          newTextureWithDescriptor:renderTargetDescriptor];

  auto *randomValues = static_cast<uint32_t *>(malloc(sizeof(uint32_t) * w * h));

  for (int i = 0; i < w * h; i++) randomValues[i] = rand() % (1024 * 1024);

  [m_randomTexture replaceRegion:MTLRegionMake2D(0, 0, w, h)
                     mipmapLevel:0
                       withBytes:randomValues
                     bytesPerRow:sizeof(uint32_t) * w];

  free(randomValues);
}

void GraphicsLayer::recordRasterArgBuffer() {
  id<MTLDevice> device = m_engine->m_renderingContext->getDevice();

  // args buffer
  id<MTLFunction> fn = m_engine->m_shaderManager->getVertexFunction(m_shaderHandle);
  id<MTLArgumentEncoder> argumentEncoder = [fn newArgumentEncoderWithBufferIndex:0];

  id<MTLFunction> fnFrag = m_engine->m_shaderManager->getFragmentFunction(m_shaderHandle);
  id<MTLArgumentEncoder> argumentEncoderFrag =
          [fnFrag newArgumentEncoderWithBufferIndex:0];

  int meshesCount = m_asset.models.size();
  int buffInstanceSize = argumentEncoder.encodedLength;
  m_argBuffer = [device newBufferWithLength:buffInstanceSize * meshesCount options:0];

  int buffInstanceSizeFrag = argumentEncoderFrag.encodedLength;
  m_argBufferFrag =
          [device newBufferWithLength:buffInstanceSizeFrag * meshesCount options:0];


  for (int i = 0; i < meshesCount; ++i) {
    [argumentEncoder setArgumentBuffer:m_argBuffer offset:i * buffInstanceSize];
    const auto *meshData = m_engine->m_meshManager->getMeshData(m_asset.models[i].mesh);
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

    const auto &material = m_asset.materials[i];
    [argumentEncoderFrag setArgumentBuffer:m_argBufferFrag
                                    offset:i * buffInstanceSizeFrag];

    id albedo;
    albedo = m_engine->m_textureManager->getNativeFromHandle(m_lightMapper.m_lightMap);
    //}
    [argumentEncoderFrag setTexture:albedo atIndex:0];
    auto *ptr = [argumentEncoderFrag constantDataAtIndex:1];
    float matData[8]{};
    matData[0] = material.colorFactors.x;
    matData[1] = material.colorFactors.y;
    matData[2] = material.colorFactors.z;
    matData[3] = material.colorFactors.w;

    //uv offset for lightmap
    const auto &packResult = m_lightMapper.getPackResult();

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


void GraphicsLayer::doRasterRender(id<MTLRenderCommandEncoder> commandEncoder,
                                   const SirMetal::PSOCache &cache) {
  [commandEncoder setRenderPipelineState:cache.color];
  [commandEncoder setDepthStencilState:cache.depth];
  [commandEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
  [commandEncoder setCullMode:MTLCullModeNone];

  SirMetal::BindInfo info =
          m_engine->m_constantBufferManager->getBindInfo(m_engine, m_camUniformHandle);
  [commandEncoder setVertexBuffer:info.buffer offset:info.offset atIndex:4];

  //setting the arguments buffer
  [commandEncoder setVertexBuffer:m_argBuffer offset:0 atIndex:0];
  [commandEncoder setFragmentBuffer:m_argBufferFrag offset:0 atIndex:0];

  int counter = 0;
  for (auto &mesh : m_asset.models) {
    if (!mesh.mesh.isHandleValid()) continue;
    const SirMetal::MeshData *meshData = m_engine->m_meshManager->getMeshData(mesh.mesh);
    auto *data = (void *) (&mesh.matrix);
    //[commandEncoder useResource: m_engine->m_textureManager->getNativeFromHandle(mesh.material.colorTexture) usage:MTLResourceUsageSample];
    [commandEncoder setVertexBytes:data length:16 * 4 atIndex:5];
    [commandEncoder setVertexBytes:&counter length:4 atIndex:6];
    [commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                               indexCount:meshData->primitivesCount
                                indexType:MTLIndexTypeUInt32
                              indexBuffer:meshData->indexBuffer
                        indexBufferOffset:0];
    counter++;
  }
}

}// namespace Sandbox
