#include "graphicsLayer.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <SirMetal/MBEMathUtilities.h>
#include <SirMetal/resources/textureManager.h>
#include <simd/simd.h>

#include "SirMetal/application/window.h"
#include "SirMetal/core/input.h"
#include "SirMetal/engine.h"
#include "SirMetal/graphics/PSOGenerator.h"
#include "SirMetal/graphics/constantBufferManager.h"
#include "SirMetal/graphics/debug/imgui/imgui.h"
#include "SirMetal/graphics/debug/imguiRenderer.h"
#include "SirMetal/graphics/debug/debugRenderer.h"
#include "SirMetal/graphics/materialManager.h"
#include "SirMetal/graphics/renderingContext.h"
#include "SirMetal/resources/meshes/meshManager.h"
#include "SirMetal/resources/shaderManager.h"

struct DirLight {

  matrix_float4x4 V;
  matrix_float4x4 P;
  matrix_float4x4 VP;
  simd_float4 lightDir;
};

namespace Sandbox {
void GraphicsLayer::onAttach(SirMetal::EngineContext *context) {

  m_engine = context;
  // initializing the camera to the identity
  m_camera.viewMatrix = matrix_float4x4_translation(vector_float3{0, 0, 0});
  m_camera.fov = M_PI / 4;
  m_camera.nearPlane = 0.01;
  m_camera.farPlane = 60;
  m_cameraController.setCamera(&m_camera);
  m_cameraController.setPosition(3, 5, 15);
  m_camConfig = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.2, 0.002};

  m_uniformHandle = m_engine->m_constantBufferManager->allocate(
      m_engine, sizeof(SirMetal::Camera),
      SirMetal::CONSTANT_BUFFER_FLAGS_BITS::CONSTANT_BUFFER_FLAG_BUFFERED);
  m_lightHandle = m_engine->m_constantBufferManager->allocate(
      m_engine, sizeof(DirLight),
      SirMetal::CONSTANT_BUFFER_FLAGS_BITS::CONSTANT_BUFFER_FLAG_NONE);
  updateLightData();

  const std::string base = m_engine->m_config.m_dataSourcePath + "/sandbox";

  const char *names[5] = {"/plane.obj", "/cube.obj", "/lucy.obj", "/cone.obj",
                          "/cilinder.obj"};
  for (int i = 0; i < 5; ++i) {
    m_meshes[i] = m_engine->m_meshManager->loadMesh(base + names[i]);
  }

  id<MTLDevice> device = m_engine->m_renderingContext->getDevice();

  SirMetal::AllocTextureRequest requestDepth{
      m_engine->m_config.m_windowConfig.m_width,
      m_engine->m_config.m_windowConfig.m_height,
      1,
      MTLTextureType2D,
      MTLPixelFormatDepth32Float_Stencil8,
      MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead,
      MTLStorageModePrivate,
      1,
      "depthTexture"};
  m_depthHandle = m_engine->m_textureManager->allocate(device, requestDepth);
  requestDepth.width = 2048;
  requestDepth.height = 2048;
  requestDepth.format = MTLPixelFormatDepth32Float;
  requestDepth.name = "shadowMap";
  m_shadowHandle = m_engine->m_textureManager->allocate(device, requestDepth);

  m_shaderHandle =
      m_engine->m_shaderManager->loadShader((base + "/Shaders.metal").c_str());

  m_shadowShaderHandle =
      m_engine->m_shaderManager->loadShader((base + "/shadows.metal").c_str());

  SirMetal::graphics::initImgui(m_engine);
}

void GraphicsLayer::onDetach() {}

void GraphicsLayer::updateUniformsForView(float screenWidth,
                                          float screenHeight) {

  // const matrix_float4x4 modelMatrix =
  //    matrix_float4x4_translation(vector_float3{0, 0, 0});
  // uniforms.modelViewProjectionMatrix =
  //    matrix_multiply(m_camera.VP, modelMatrix);
  SirMetal::Input *input = m_engine->m_inputManager;
  m_cameraController.update(m_camConfig, input);
  // uniforms.modelViewProjectionMatrix =
  //    matrix_multiply(m_camera.VP, modelMatrix);

  m_cameraController.updateProjection(screenWidth, screenHeight);
  m_engine->m_constantBufferManager->update(m_engine, m_uniformHandle,
                                            &m_camera);
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

  //@autoreleasepool {
  id<CAMetalDrawable> surface = [swapchain nextDrawable];
  id<MTLTexture> texture = surface.texture;

  float w = texture.width;
  float h = texture.height;
  updateUniformsForView(w, h);

  id<MTLDevice> device = m_engine->m_renderingContext->getDevice();
  SirMetal::graphics::DrawTracker shadowTracker{};
  shadowTracker.renderTargets[0] = nil;
  shadowTracker.depthTarget =
      m_engine->m_textureManager->getNativeFromHandle(m_shadowHandle);
  SirMetal::PSOCache cache = SirMetal::getPSO(
      m_engine, shadowTracker, SirMetal::Material{"shadows", false});

  id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
  [commandBuffer setLabel:@"testName"];
  // shadows
  MTLRenderPassDescriptor *shadowPassDescriptor =
      [MTLRenderPassDescriptor renderPassDescriptor];
  shadowPassDescriptor.colorAttachments[0].texture = nil;
  shadowPassDescriptor.colorAttachments[0].clearColor = {};
  shadowPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionDontCare;
  shadowPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionDontCare;

  MTLRenderPassDepthAttachmentDescriptor *depthAttachment =
      shadowPassDescriptor.depthAttachment;
  depthAttachment.texture =
      m_engine->m_textureManager->getNativeFromHandle(m_shadowHandle);
  depthAttachment.clearDepth = 1.0;
  depthAttachment.storeAction = MTLStoreActionDontCare;
  depthAttachment.loadAction = MTLLoadActionClear;

  // shadow pass
  id<MTLRenderCommandEncoder> shadowEncoder =
      [commandBuffer renderCommandEncoderWithDescriptor:shadowPassDescriptor];

  [shadowEncoder setRenderPipelineState:cache.color];
  [shadowEncoder setDepthStencilState:cache.depth];
  [shadowEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
  [shadowEncoder setCullMode:MTLCullModeBack];

  SirMetal::BindInfo info =
      m_engine->m_constantBufferManager->getBindInfo(m_engine, m_lightHandle);
  [shadowEncoder setVertexBuffer:info.buffer offset:info.offset atIndex:4];

  for (auto &mesh : m_meshes) {
    const SirMetal::MeshData *meshData =
        m_engine->m_meshManager->getMeshData(mesh);

    [shadowEncoder setVertexBuffer:meshData->vertexBuffer
                            offset:meshData->ranges[0].m_offset
                           atIndex:0];
    [shadowEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                              indexCount:meshData->primitivesCount
                               indexType:MTLIndexTypeUInt32
                             indexBuffer:meshData->indexBuffer
                       indexBufferOffset:0];
  }
  [shadowEncoder endEncoding];

  // render
  SirMetal::graphics::DrawTracker tracker{};
  tracker.renderTargets[0] = texture;
  tracker.depthTarget =
      m_engine->m_textureManager->getNativeFromHandle(m_depthHandle);
  cache =
      SirMetal::getPSO(m_engine, tracker, SirMetal::Material{"Shaders", false});

  MTLRenderPassDescriptor *passDescriptor =
      [MTLRenderPassDescriptor renderPassDescriptor];

  passDescriptor.colorAttachments[0].texture = texture;
  passDescriptor.colorAttachments[0].clearColor = {0.2, 0.2, 0.2, 1.0};
  passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
  passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;

  depthAttachment = passDescriptor.depthAttachment;
  depthAttachment.texture =
      m_engine->m_textureManager->getNativeFromHandle(m_depthHandle);
  depthAttachment.clearDepth = 1.0;
  depthAttachment.storeAction = MTLStoreActionDontCare;
  depthAttachment.loadAction = MTLLoadActionClear;
  id<MTLRenderCommandEncoder> commandEncoder =
      [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];

  [commandEncoder setRenderPipelineState:cache.color];
  [commandEncoder setDepthStencilState:cache.depth];
  [commandEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
  [commandEncoder setCullMode:MTLCullModeBack];

  info =
      m_engine->m_constantBufferManager->getBindInfo(m_engine, m_uniformHandle);
  [commandEncoder setVertexBuffer:info.buffer offset:info.offset atIndex:4];
  info =
      m_engine->m_constantBufferManager->getBindInfo(m_engine, m_lightHandle);
  [commandEncoder setFragmentBuffer:info.buffer offset:info.offset atIndex:5];
  id<MTLTexture> shadow = m_engine->m_textureManager->getNativeFromHandle(m_shadowHandle);
  [commandEncoder setFragmentTexture:shadow atIndex:0];

  for (auto &mesh : m_meshes) {
    const SirMetal::MeshData *meshData =
        m_engine->m_meshManager->getMeshData(mesh);

    [commandEncoder setVertexBuffer:meshData->vertexBuffer
                             offset:meshData->ranges[0].m_offset
                            atIndex:0];
    [commandEncoder setVertexBuffer:meshData->vertexBuffer
                             offset:meshData->ranges[1].m_offset
                            atIndex:1];
    [commandEncoder setVertexBuffer:meshData->vertexBuffer
                             offset:meshData->ranges[2].m_offset
                            atIndex:2];
    [commandEncoder setVertexBuffer:meshData->vertexBuffer
                             offset:meshData->ranges[3].m_offset
                            atIndex:3];
    [commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                               indexCount:meshData->primitivesCount
                                indexType:MTLIndexTypeUInt32
                              indexBuffer:meshData->indexBuffer
                        indexBufferOffset:0];
  }
  //render debug
  m_engine->m_debugRenderer->newFrame();
  float data[6]{0,0,0,0,100,0};
  m_engine->m_debugRenderer->drawLines(data,sizeof(float)*6,vector_float4{1,0,0,1});
  m_engine->m_debugRenderer->render(m_engine,commandEncoder,tracker,m_uniformHandle,300,300);

  // ui
  SirMetal::graphics::imguiNewFrame(m_engine, passDescriptor);
  //ImGui::ShowDemoWindow((bool *)0);
  SirMetal::graphics::imguiEndFrame(commandBuffer, commandEncoder);

  [commandEncoder endEncoding];

  [commandBuffer presentDrawable:surface];
  [commandBuffer commit];
  //}
}

bool GraphicsLayer::onEvent(SirMetal::Event &) {
  // TODO start processing events the game cares about, like
  // etc...
  return false;
}

void GraphicsLayer::clear() { SirMetal::graphics::shutdownImgui(); }
void GraphicsLayer::updateLightData() {
  DirLight light{};
  simd_float3 view{0, 1, 1};
  simd_float3 up{0, 1, 0};
  simd_float3 cross = simd_normalize(simd_cross(up,view));
  up = simd_normalize(simd_cross(view,cross ));
  simd_float4 pos4{0, 15, 15, 1};

  simd_float4 view4{view.x, view.y, view.z, 0};
  simd_float4 up4{up.x, up.y, up.z, 0};
  simd_float4 cross4{cross.x, cross.y, cross.z, 0};
  light.V = {cross4, up4, view4, pos4};
  light.P = matrix_float4x4_perspective(1, M_PI / 2, 0.01f, 40.0f);
  light.VP = simd_mul(light.P, simd_inverse(light.V));
  light.lightDir = view4;

  m_engine->m_constantBufferManager->update(m_engine, m_lightHandle, &light);
}
} // namespace Sandbox
