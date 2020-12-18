#include "graphicsLayer.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <SirMetal/core/mathUtils.h>
#include <SirMetal/resources/textureManager.h>
#include <simd/simd.h>

#include "SirMetal/application/window.h"
#include "SirMetal/core/input.h"
#include "SirMetal/engine.h"
#include "SirMetal/graphics/PSOGenerator.h"
#include "SirMetal/graphics/constantBufferManager.h"
#include "SirMetal/graphics/materialManager.h"
#include "SirMetal/graphics/renderingContext.h"
#include "SirMetal/resources/meshes/meshManager.h"
#include "SirMetal/resources/shaderManager.h"

#include <iostream>

typedef struct {
  matrix_float4x4 modelViewProjectionMatrix;
} MBEUniforms;

void GraphicsLayer::onAttach(SirMetal::EngineContext *context) {

  m_engine = context;
  // initializing the camera to the identity
  m_camera.viewMatrix = matrix_float4x4_translation(vector_float3{0, 0, 0});
  m_camera.fov = M_PI / 4;
  m_camera.nearPlane = 1;
  m_camera.farPlane = 100;
  m_cameraController.setCamera(&m_camera);
  m_cameraController.setPosition(0, 0, 5);
  m_camConfig = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.2, 0.002};

  m_uniformHandle = m_engine->m_constantBufferManager->allocate(
      m_engine, sizeof(MBEUniforms),
      SirMetal::CONSTANT_BUFFER_FLAGS_BITS::CONSTANT_BUFFER_FLAG_BUFFERED);

  auto sourcePath = m_engine->m_config.m_dataSourcePath;
  auto baseSample = sourcePath + "/01_jumpFlooding";
  m_mesh = m_engine->m_meshManager->loadMesh(baseSample + "/lucy.obj");

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

  m_engine->m_shaderManager->loadShader((baseSample + "/Shaders.metal").c_str());
  m_engine->m_shaderManager->loadShader(
      (baseSample + "/jumpInit.metal").c_str());
  m_engine->m_shaderManager->loadShader(
      (baseSample + "/jumpMask.metal").c_str());
  m_engine->m_shaderManager->loadShader(
      (baseSample + "/jumpFlood.metal").c_str());
  m_engine->m_shaderManager->loadShader(
      (baseSample + "/jumpOutline.metal").c_str());
  m_selection.initialize(context);
}

void GraphicsLayer::onDetach() {}

void GraphicsLayer::updateUniformsForView(float screenWidth,
                                          float screenHeight) {

  const matrix_float4x4 modelMatrix =
      matrix_float4x4_translation(vector_float3{0, 0, 0});
  MBEUniforms uniforms;
  uniforms.modelViewProjectionMatrix =
      matrix_multiply(m_camera.VP, modelMatrix);
  SirMetal::Input *input = m_engine->m_inputManager;
  m_cameraController.update(m_camConfig, input);
  uniforms.modelViewProjectionMatrix =
      matrix_multiply(m_camera.VP, modelMatrix);

  m_cameraController.updateProjection(screenWidth, screenHeight);
  m_engine->m_constantBufferManager->update(m_engine, m_uniformHandle,
                                            &uniforms);
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

  SirMetal::graphics::DrawTracker tracker{};

  id<MTLDevice> device = m_engine->m_renderingContext->getDevice();
  tracker.renderTargets[0] = texture;
  tracker.depthTarget =
      m_engine->m_textureManager->getNativeFromHandle(m_depthHandle);
  SirMetal::PSOCache cache =
      SirMetal::getPSO(m_engine, tracker, SirMetal::Material{"Shaders", false});

  MTLRenderPassDescriptor *passDescriptor =
      [MTLRenderPassDescriptor renderPassDescriptor];
  passDescriptor.colorAttachments[0].texture = texture;
  passDescriptor.colorAttachments[0].clearColor =
      MTLClearColorMake(0.85, 0.85, 0.85, 1);
  passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
  passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;

  MTLRenderPassDepthAttachmentDescriptor *depthAttachment =
      passDescriptor.depthAttachment;
  depthAttachment.texture =
      m_engine->m_textureManager->getNativeFromHandle(m_depthHandle);
  depthAttachment.clearDepth = 1.0;
  depthAttachment.storeAction = MTLStoreActionDontCare;
  depthAttachment.loadAction = MTLLoadActionClear;

  id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];

  id<MTLRenderCommandEncoder> commandEncoder =
      [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];

  [commandEncoder setRenderPipelineState:cache.color];
  [commandEncoder setDepthStencilState:cache.depth];
  [commandEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
  [commandEncoder setCullMode:MTLCullModeBack];

  SirMetal::BindInfo info =
      m_engine->m_constantBufferManager->getBindInfo(m_engine, m_uniformHandle);
  [commandEncoder setVertexBuffer:info.buffer offset:info.offset atIndex:4];

  const SirMetal::MeshData *meshData =
      m_engine->m_meshManager->getMeshData(m_mesh);

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
  [commandEncoder endEncoding];

  m_selection.render(m_engine, commandBuffer, w, h, m_uniformHandle, texture);

  [commandBuffer presentDrawable:surface];
  [commandBuffer commit];
  //}
}

bool GraphicsLayer::onEvent(SirMetal::Event &) {
  // TODO start processing events the game cares about, like
  // etc...
  return false;
}

void GraphicsLayer::clear() {}
