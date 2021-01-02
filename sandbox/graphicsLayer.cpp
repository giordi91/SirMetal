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
//resources
#include <SirMetal/resources/textureManager.h>
#include "SirMetal/resources/meshes/meshManager.h"
#include "SirMetal/resources/shaderManager.h"

#include "finders_interface.h"//rectpack2D

static const char *rts[] = {"GPositions", "GUVs", "GNormals", "lightMap"};

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

  SirMetal::loadGLTF(m_engine, (base + +"/test.glb").c_str(), asset, options);

  packResult = buildPacking(16384, lightMapSize, asset.models.size());

  id<MTLDevice> device = m_engine->m_renderingContext->getDevice();

  assert(device.supportsRaytracing == true &&
         "This device does not support raytracing API, you can try samples based on MPS");

  allocateGBufferTexture(packResult.w, packResult.h);

  m_shaderHandle =
          m_engine->m_shaderManager->loadShader((base + "/Shaders.metal").c_str());
  m_gbuffHandle = m_engine->m_shaderManager->loadShader((base + "/gbuff.metal").c_str());
  m_fullScreenHandle =
          m_engine->m_shaderManager->loadShader((base + "/fullscreen.metal").c_str());
  m_rtLightMapHandle =
          m_engine->m_shaderManager->loadShader((base + "/rtLightMap.metal").c_str());

#if RT
  recordRTArgBuffer();
#endif
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

  rtLightmapPipeline = createComputePipeline(
          device, m_engine->m_shaderManager->getKernelFunction(m_rtLightMapHandle));
  // this is to flush the gpu, should figure out why is not actually flushing
  // properly
  m_engine->m_renderingContext->flush();


#if RT
  SirMetal::graphics::buildMultiLevelBVH(m_engine, asset.models, accelStruct);
#endif

  //generateRandomTexture(1280u, 720u);
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

  u.frameIndex = rtSampleCounter;
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


  //RASTER
  //since we cannot shoot rays from the fragment shader we are going to make a gbuffer pass
  //storing world position, uvs and normals
  doGBufferPass(commandBuffer);

  //now that we have that we can actually kick a raytracing shader which uses the gbuffer information
  //for the first ray
#if RT
  if (rtSampleCounter < requestedSamples) { doLightmapBake(commandBuffer); }
#endif


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
        request.srcTexture = m_gbuff[0];
        break;
      case 1:
        request.srcTexture = m_gbuff[1];
        break;
      case 2:
        request.srcTexture = m_gbuff[2];
        break;
      case 3:
        request.srcTexture = m_lightMap;
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

      ImGui::SliderInt("Number of samples", &requestedSamples, 0, 4000);
      ImGui::Separator();
      //bake button
      bool isDone = rtSampleCounter == requestedSamples;
      if (isDone) { ImGui::PushStyleColor(0, ImVec4(0, 1, 0, 1)); }
      if (ImGui::Button("Bake lightmap")) { rtSampleCounter = 0; }
      if (isDone) { ImGui::PopStyleColor(1); }

      ImGui::PushItemWidth(50.0f);
      ImGui::DragInt("Samples done", &rtSampleCounter, 0.0f);
      ImGui::PopItemWidth();
      ImGui::Separator();

      ImGui::Checkbox("Debug full screen", &debugFullScreen);
      if (debugFullScreen) { ImGui::Combo("Target", &currentDebug, rts, 4); }
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

#if RT
void GraphicsLayer::recordRTArgBuffer() {
  id<MTLDevice> device = m_engine->m_renderingContext->getDevice();

  // args buffer
  id<MTLFunction> fn = m_engine->m_shaderManager->getKernelFunction(m_rtLightMapHandle);
  id<MTLArgumentEncoder> argumentEncoder = [fn newArgumentEncoderWithBufferIndex:2];

  int meshesCount = asset.models.size();
  int buffInstanceSize = argumentEncoder.encodedLength;
  argRtBuffer = [device newBufferWithLength:buffInstanceSize * meshesCount options:0];

  for (int i = 0; i < meshesCount; ++i) {
    [argumentEncoder setArgumentBuffer:argRtBuffer offset:i * buffInstanceSize];
    const auto *meshData = m_engine->m_meshManager->getMeshData(asset.models[i].mesh);
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
    const auto &material = asset.materials[i];
    id albedo = m_engine->m_textureManager->getNativeFromHandle(material.colorTexture);
    [argumentEncoder setTexture:albedo atIndex:5];

    auto *ptrMatrix = [argumentEncoder constantDataAtIndex:6];
    memcpy(ptrMatrix, &asset.models[i].matrix, sizeof(float) * 16);
    memcpy(((char *) ptrMatrix + sizeof(float) * 16), &material.colorFactors,
           sizeof(float) * 4);
  }
}
#endif
void GraphicsLayer::recordRasterArgBuffer() {
  id<MTLDevice> device = m_engine->m_renderingContext->getDevice();

  // args buffer
  id<MTLFunction> fn = m_engine->m_shaderManager->getVertexFunction(m_shaderHandle);
  id<MTLArgumentEncoder> argumentEncoder = [fn newArgumentEncoderWithBufferIndex:0];

  id<MTLFunction> fnFrag = m_engine->m_shaderManager->getFragmentFunction(m_shaderHandle);
  id<MTLArgumentEncoder> argumentEncoderFrag =
          [fnFrag newArgumentEncoderWithBufferIndex:0];

  int meshesCount = asset.models.size();
  int buffInstanceSize = argumentEncoder.encodedLength;
  argBuffer = [device newBufferWithLength:buffInstanceSize * meshesCount options:0];

  int buffInstanceSizeFrag = argumentEncoderFrag.encodedLength;
  argBufferFrag =
          [device newBufferWithLength:buffInstanceSizeFrag * meshesCount options:0];


  for (int i = 0; i < meshesCount; ++i) {
    [argumentEncoder setArgumentBuffer:argBuffer offset:i * buffInstanceSize];
    const auto *meshData = m_engine->m_meshManager->getMeshData(asset.models[i].mesh);
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

    const auto &material = asset.materials[i];
    [argumentEncoderFrag setArgumentBuffer:argBufferFrag offset:i * buffInstanceSizeFrag];

    id albedo;
    albedo = m_engine->m_textureManager->getNativeFromHandle(m_lightMap);
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
void GraphicsLayer::allocateGBufferTexture(int w, int h) {


  id<MTLDevice> device = m_engine->m_renderingContext->getDevice();
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
  m_gbuff[0] = m_engine->m_textureManager->allocate(device, request);
  auto oldUsage = request.usage;
  request.usage = oldUsage | MTLTextureUsageShaderWrite;
  request.name = "lightMap1";
  m_lightMap = m_engine->m_textureManager->allocate(device, request);
  request.usage = oldUsage;
  request.format = MTLPixelFormatRG16Unorm;
  request.name = "gbuffUVs";
  m_gbuff[1] = m_engine->m_textureManager->allocate(device, request);
  request.format = MTLPixelFormatRGBA8Unorm;
  request.name = "gbuffNormals";
  m_gbuff[2] = m_engine->m_textureManager->allocate(device, request);
}
void GraphicsLayer::doGBufferPass(id<MTLCommandBuffer> commandBuffer) {

  SirMetal::graphics::DrawTracker tracker{};
  id g1 = m_engine->m_textureManager->getNativeFromHandle(m_gbuff[0]);
  id g2 = m_engine->m_textureManager->getNativeFromHandle(m_gbuff[1]);
  id g3 = m_engine->m_textureManager->getNativeFromHandle(m_gbuff[2]);
  tracker.renderTargets[0] = g1;
  tracker.renderTargets[1] = g2;
  tracker.renderTargets[2] = g3;
  tracker.depthTarget = {};
  //  m_engine->m_textureManager->getNativeFromHandle(m_depthHandle);

  SirMetal::PSOCache cache =
          SirMetal::getPSO(m_engine, tracker, SirMetal::Material{"gbuff", false});

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

  SirMetal::BindInfo info =
          m_engine->m_constantBufferManager->getBindInfo(m_engine, m_camUniformHandle);
  [commandEncoder setVertexBuffer:info.buffer offset:info.offset atIndex:4];
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
  jitter[0] = cos(t1) / lightMapSize;
  jitter[1] = sin(t2) / lightMapSize;
  jitter[0] *= (jitterMultiplier);
  jitter[1] *= (jitterMultiplier);

  for (int i = 0; i < asset.models.size(); ++i) {
    const auto &mesh = asset.models[i];
    const SirMetal::MeshData *meshData = m_engine->m_meshManager->getMeshData(mesh.mesh);
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
    [commandEncoder useResource:m_engine->m_textureManager->getNativeFromHandle(
                                        asset.materials[i].colorTexture)
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
    rect.width = m_engine->m_config.m_windowConfig.m_width;
    rect.height = m_engine->m_config.m_windowConfig.m_height;
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

#if RT
void GraphicsLayer::doLightmapBake(id<MTLCommandBuffer> commandBuffer) {

  int index = m_engine->m_timings.m_totalNumberOfFrames % asset.models.size();
  //HARDCODED HEIGHT
  int w = lightMapSize;
  int h = lightMapSize;
  MTLSize threadsPerThreadgroup = MTLSizeMake(8, 8, 1);
  MTLSize threadgroups = MTLSizeMake(
          (w + threadsPerThreadgroup.width - 1) / threadsPerThreadgroup.width,
          (h + threadsPerThreadgroup.height - 1) / threadsPerThreadgroup.height, 1);

  id<MTLComputeCommandEncoder> computeEncoder = [commandBuffer computeCommandEncoder];

  auto bindInfo = m_engine->m_constantBufferManager->getBindInfo(m_engine, m_uniforms);
  id<MTLTexture> colorTexture =
          m_engine->m_textureManager->getNativeFromHandle(m_lightMap);

  id g1 = m_engine->m_textureManager->getNativeFromHandle(m_gbuff[0]);
  id g2 = m_engine->m_textureManager->getNativeFromHandle(m_gbuff[1]);
  id g3 = m_engine->m_textureManager->getNativeFromHandle(m_gbuff[2]);


  [computeEncoder setAccelerationStructure:accelStruct.instanceAccelerationStructure
                             atBufferIndex:0];
  [computeEncoder setBuffer:bindInfo.buffer offset:bindInfo.offset atIndex:1];
  [computeEncoder setBuffer:argRtBuffer offset:0 atIndex:2];
  [computeEncoder setBytes:&index length:4 atIndex:3];
  int off[] = {packResult.rectangles[index].x, packResult.rectangles[index].y};
  [computeEncoder setBytes:&off[0] length:sizeof(int) * 2 atIndex:4];
  [computeEncoder setTexture:colorTexture atIndex:0];
  [computeEncoder setTexture:m_randomTexture atIndex:1];
  [computeEncoder setTexture:g1 atIndex:3];
  [computeEncoder setTexture:g2 atIndex:4];
  [computeEncoder setTexture:g3 atIndex:5];
  [computeEncoder setComputePipelineState:rtLightmapPipeline];
  [computeEncoder dispatchThreadgroups:threadgroups
                 threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];
  [computeEncoder endEncoding];

  ++rtFrameCounterFull;
  if ((rtFrameCounterFull % asset.models.size()) == 0) { rtSampleCounter++; }
}
#endif

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
  [commandEncoder setVertexBuffer:argBuffer offset:0 atIndex:0];
  [commandEncoder setFragmentBuffer:argBufferFrag offset:0 atIndex:0];

  int counter = 0;
  for (auto &mesh : asset.models) {
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


PackingResult GraphicsLayer::buildPacking(int maxSize, int individualSize, int count) {
  const auto runtime_flipping_mode = rectpack2D::flipping_option::DISABLED;

  /*
        Here, we choose the "empty_spaces" class that the algorithm will use from now on.

        The first template argument is a bool which determines
        if the algorithm will try to flip rectangles to better fit them.
        The second argument is optional and specifies an allocator for the empty spaces.
        The default one just uses a vector to store the spaces.
        You can also pass a "static_empty_spaces<10000>" which will allocate 10000 spaces on the stack,
        possibly improving performance.
    */
  constexpr bool allow_flip = false;
  using spaces_type =
          rectpack2D::empty_spaces<allow_flip, rectpack2D::default_empty_spaces>;

  /*
        rect_xywh or rect_xywhf (see src/rect_structs.h),
        depending on the value of allow_flip.
    */

  using rect_type = rectpack2D::output_rect_t<spaces_type>;

  /*
        Note:
        The multiple-bin functionality was removed.
        This means that it is now up to you what is to be done with unsuccessful insertions.
        You may initialize another search when this happens.
    */

  auto report_successful = [](rect_type &) {
    return rectpack2D::callback_result::CONTINUE_PACKING;
  };

  auto report_unsuccessful = [](rect_type &) {
    return rectpack2D::callback_result::ABORT_PACKING;
  };


  /*
        The search stops when the bin was successfully inserted into,
        AND the next candidate bin size differs from the last successful one by *less* then discard_step.
        The best possible granuarity is achieved with discard_step = 1.
        If you pass a negative discard_step, the algoritm will search with even more granularity -
        E.g. with discard_step = -4, the algoritm will behave as if you passed discard_step = 1,
        but it will make as many as 4 attempts to optimize bins down to the single pixel.
        Since discard_step = 0 does not make sense, the algoritm will automatically treat this case
        as if it were passed a discard_step = 1.
        For common applications, a discard_step = 1 or even discard_step = 128
        should yield really good packings while being very performant.
        If you are dealing with very small rectangles specifically,
        it might be a good idea to make this value negative.
        See the algorithm section of README for more information.
    */

  const auto discard_step = -4;

  /*
        Create some arbitrary rectangles.
        Every subsequent call to the packer library will only read the widths and heights that we now specify,
        and always overwrite the x and y coordinates with calculated results.
    */

  std::vector<rect_type> rectangles;

  for (int i = 0; i < count; ++i) {
    rectangles.emplace_back(rectpack2D::rect_xywh(0, 0, individualSize, individualSize));
  }

  auto report_result = [&rectangles](const rectpack2D::rect_wh &result_size) {
    printf("Resultant bin: %i %i \n", result_size.w, result_size.h);

    for (const auto &r : rectangles) { printf("%i %i %i %i\n", r.x, r.y, r.w, r.h); }
  };

  {
    /*
          Example 1: Find best packing with default orders.
          If you pass no comparators whatsoever,
          the standard collection of 6 orders:
             by area, by perimeter, by bigger side, by width, by height and by "pathological multiplier"
          - will be passed by default.
      */

    //const auto result_size = rectpack2D::find_best_packing<spaces_type>(
    //        rectangles, make_finder_input(max_side, discard_step, report_successful,
    //                                      report_unsuccessful, runtime_flipping_mode));

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

}// namespace Sandbox
