#include "graphicsLayer.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <SirMetal/MBEMathUtilities.h>
#include <simd/simd.h>
#include <simd/types.h>

#include "SirMetal/application/window.h"
#include "SirMetal/core/input.h"
#include "SirMetal/engine.h"
#include "SirMetal/graphics/renderingContext.h"

static id<MTLRenderPipelineState> renderPipelineState;
static id<MTLDepthStencilState> depthStencilState;
static id<MTLBuffer> vertexBuffer;
static id<MTLBuffer> uniformBuffer;

static simd_float4x4 viewMatrix;
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
static const NSInteger MBEInFlightBufferCount = 3;
static const NSInteger bufferIndex = 0;
const MTLIndexType MBEIndexType = MTLIndexTypeUInt32;
namespace Sandbox {
void GraphicsLayer::onAttach(SirMetal::EngineContext *context) {

  m_engine = context;
  color = MTLClearColorMake(0, 0, 0, 1);
  makePipeline();
  makeBuffers();

  // view matrix
  viewMatrix = matrix_float4x4_translation(vector_float3{0, 0, 5});
  // initializing the camera to the identity
  m_camera.viewMatrix = matrix_float4x4_translation(vector_float3{0, 0, 0});
  m_camera.fov = M_PI / 4;
  m_camera.nearPlane = 1;
  m_camera.farPlane = 100;
  m_cameraController.setCamera(&m_camera);
  m_cameraController.setPosition(0, 0, 5);
  m_camConfig = {1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 0.2, 0.002};

  id<MTLDevice> device = m_engine->m_renderingContext->getDevice();
  uniformBuffer = [device newBufferWithLength:AlignUp(sizeof(MBEUniforms), 256) *3
  options:MTLResourceOptionCPUCacheModeDefault];
  [uniformBuffer setLabel:@"Uniforms"];
}

void GraphicsLayer::onDetach() {}

void GraphicsLayer::makePipeline() {
  const char *shader = "Shaders.metal";
  NSString *shaderPath =
      [NSString stringWithCString:shader
                         encoding:[NSString defaultCStringEncoding]];
  __autoreleasing NSError *errorLib = nil;

  /*
  const char *cString = [shaderPath cStringUsingEncoding:NSASCIIStringEncoding];
  std::ifstream t(shader);
  std::string str((std::istreambuf_iterator<char>(t)),
                  std::istreambuf_iterator<char>());
  const char *shaderData = readFile(t);
   */

  id<MTLDevice> device = m_engine->m_renderingContext->getDevice();

  NSString *content = [NSString stringWithContentsOfFile:shaderPath
                                                encoding:NSUTF8StringEncoding
                                                   error:nil];
  id<MTLLibrary> libraryRaw =
      [device newLibraryWithSource:content options:nil error:&errorLib];

  id<MTLFunction> vertexFunc = [libraryRaw newFunctionWithName:@"vertex_main"];
  id<MTLFunction> fragmentFunc =
      [libraryRaw newFunctionWithName:@"fragment_main"];

  MTLRenderPipelineDescriptor *pipelineDescriptor =
      [MTLRenderPipelineDescriptor new];
  pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
  pipelineDescriptor.vertexFunction = vertexFunc;
  pipelineDescriptor.fragmentFunction = fragmentFunc;

  NSError *error = nil;
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

  static const uint32_t MBEBufferAlignment = 256;
  m_cameraController.updateProjection(screenWidth, screenHeight);
  const NSUInteger uniformBufferOffset =
      AlignUp(sizeof(MBEUniforms), MBEBufferAlignment) * bufferIndex;
  memcpy((char *)([uniformBuffer contents]) + uniformBufferOffset, &uniforms,
         sizeof(uniforms));
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
    SDL_Event sdlevent;
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
    updateUniformsForView(w,h);
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

        const NSUInteger uniformBufferOffset = AlignUp(sizeof(MBEUniforms), 256) * bufferIndex;
      [commandEncoder setRenderPipelineState:renderPipelineState];
        [commandEncoder setVertexBuffer:uniformBuffer offset:uniformBufferOffset atIndex:1];
      [commandEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
      [commandEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                         vertexStart:0
                         vertexCount:3];
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
