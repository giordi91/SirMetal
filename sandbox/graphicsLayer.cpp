#include "graphicsLayer.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <SirMetal/MBEMathUtilities.h>
#include <simd/simd.h>
#include <simd/types.h>

#include "SirMetal/application/window.h"
#include "SirMetal/core/input.h"
#include "SirMetal/engine.h"
#include "SirMetal/graphics/constantBufferManager.h"
#include "SirMetal/graphics/renderingContext.h"
#include "SirMetal/resources/meshes/meshManager.h"
#include "SirMetal/resources/shaderManager.h"

static id<MTLRenderPipelineState> renderPipelineState;
static id<MTLDepthStencilState> depthStencilState;
static id<MTLBuffer> vertexBuffer;

typedef struct {
  vector_float4 position;
  vector_float4 color;
} MBEVertex;

typedef struct {
  matrix_float4x4 modelViewProjectionMatrix;
} MBEUniforms;

static inline uint64_t AlignUp(uint64_t n, uint32_t alignment) {
  return ((n + alignment - 1) / alignment) * alignment;
}
static const NSInteger bufferIndex = 0;

namespace Sandbox {
void GraphicsLayer::onAttach(SirMetal::EngineContext *context) {

  m_engine = context;
  color = MTLClearColorMake(0, 0, 0, 1);
  makePipeline();
  makeBuffers();

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

  m_mesh = m_engine->m_meshManager->loadMesh("lucy.obj");
}

void GraphicsLayer::onDetach() {}

void GraphicsLayer::makePipeline() {
  const char *shader = "Shaders.metal";
  SirMetal::LibraryHandle libHandle =
      m_engine->m_shaderManager->loadShader(shader);
  id<MTLFunction> vertexFunc =
      m_engine->m_shaderManager->getVertexFunction(libHandle);
  id<MTLFunction> fragmentFunc =
      m_engine->m_shaderManager->getFragmentFunction(libHandle);

  MTLRenderPipelineDescriptor *pipelineDescriptor =
      [MTLRenderPipelineDescriptor new];
  pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
  pipelineDescriptor.vertexFunction = vertexFunc;
  pipelineDescriptor.fragmentFunction = fragmentFunc;

  NSError *error = nil;
  id<MTLDevice> device = m_engine->m_renderingContext->getDevice();
  renderPipelineState =
      [device newRenderPipelineStateWithDescriptor:pipelineDescriptor
                                             error:&error];

  if (!renderPipelineState) {
    printf("[ERROR] Error occurred when creating render pipeline state:");
  }
}

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

void GraphicsLayer::makeBuffers() {
  static const MBEVertex vertices[] = {
      {.position = {0.0, 0.5, 0, 1}, .color = {1, 0, 0, 1}},
      {.position = {-0.5, -0.5, 0, 1}, .color = {0, 1, 0, 1}},
      {.position = {0.5, -0.5, 0, 1}, .color = {0, 0, 1, 1}}};

  id<MTLDevice> device = m_engine->m_renderingContext->getDevice();
  vertexBuffer =
      [device newBufferWithBytes:vertices
                          length:sizeof(vertices)
                         options:MTLResourceOptionCPUCacheModeDefault];
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

  @autoreleasepool {
    id<CAMetalDrawable> surface = [swapchain nextDrawable];
    id<MTLTexture> texture = surface.texture;

    float w = texture.width;
    float h = texture.height;
    updateUniformsForView(w, h);
    if (surface) {
      MTLRenderPassDescriptor *passDescriptor =
          [MTLRenderPassDescriptor renderPassDescriptor];
      passDescriptor.colorAttachments[0].texture = texture;
      passDescriptor.colorAttachments[0].clearColor =
          MTLClearColorMake(0.85, 0.85, 0.85, 1);
      passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
      passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;

      id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];

      id<MTLRenderCommandEncoder> commandEncoder =
          [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];

      [commandEncoder setRenderPipelineState:renderPipelineState];
      SirMetal::BindInfo info = m_engine->m_constantBufferManager->getBindInfo(
          m_engine, m_uniformHandle);
      [commandEncoder setVertexBuffer:info.buffer offset:info.offset atIndex:1];

      const SirMetal::MeshData *meshData =
          m_engine->m_meshManager->getMeshData(m_mesh);

      [commandEncoder setVertexBuffer:meshData->vertexBuffer
                               offset:0
                              atIndex:0];
      [commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                 indexCount:meshData->primitivesCount
                                 indexCount:meshData->primitivesCount
                                  indexType:MTLIndexTypeUInt32
                                indexBuffer:meshData->indexBuffer
                          indexBufferOffset:0];
      [commandEncoder endEncoding];

      [commandBuffer presentDrawable:surface];
      [commandBuffer commit];
    }

    /*
    color.red = (color.red > 1.0) ? 0 : color.red + 0.01;

    MTLRenderPassDescriptor *pass =
        [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].clearColor = color;
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].texture = surface.texture;

    id<MTLCommandBuffer> buffer = [queue commandBuffer];
    id<MTLRenderCommandEncoder> encoder =
        [buffer renderCommandEncoderWithDescriptor:pass];
    [encoder endEncoding];
    [buffer presentDrawable:surface];
    [buffer commit];
     */
  }
}

bool GraphicsLayer::onEvent(SirMetal::Event &) {
  // TODO start processing events the game cares about, like
  // etc...
  return false;
}

void GraphicsLayer::clear() {}
} // namespace Sandbox
