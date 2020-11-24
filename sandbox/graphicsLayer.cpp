#include "graphicsLayer.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <simd/types.h>

#include "SirMetal/application/window.h"
#include "SirMetal/core/input.h"
#include "SirMetal/engine.h"
static id<MTLRenderPipelineState> renderPipelineState;
static id<MTLDepthStencilState> depthStencilState;
static id<MTLBuffer> vertexBuffer;

typedef struct {
  vector_float4 position;
  vector_float4 color;
} MBEVertex;

namespace Sandbox {
void GraphicsLayer::onAttach(SirMetal::EngineContext *context) {
  /*
  camera.setLookAt(0.0f, 0.0f, 0.0f);
  camera.setPosition(0.0f, 5.0f, -10.0f);

  BlackHole::CameraManipulationConfig camConfig{
      -0.01f, 0.01f, 0.012f, 0.012f, -0.07f,
  };
  camera.setManipulationMultipliers(camConfig);
  camera.setCameraPhyisicalParameters(60.0f, 0.01f, 200.0f);

   */
  m_engine = context;
  color = MTLClearColorMake(0, 0, 0, 1);
  makePipeline();
  makeBuffers();
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

  auto device = m_engine->m_window->getGPU();

  NSString *content = [NSString stringWithContentsOfFile:shaderPath
                                                encoding:NSUTF8StringEncoding
                                                   error:nil];
  NSLog(shaderPath);
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

void GraphicsLayer::makeBuffers() {
  static const MBEVertex vertices[] = {
      {.position = {0.0, 0.5, 0, 1}, .color = {1, 0, 0, 1}},
      {.position = {-0.5, -0.5, 0, 1}, .color = {0, 1, 0, 1}},
      {.position = {0.5, -0.5, 0, 1}, .color = {0, 0, 1, 1}}};

  auto device = m_engine->m_window->getGPU();
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

  CAMetalLayer *swapchain = m_engine->m_window->getSwapchain();
  id<MTLCommandQueue> queue = m_engine->m_window->getQueue();

  @autoreleasepool {
    id<CAMetalDrawable> surface = [swapchain nextDrawable];
    id<MTLTexture> texture = surface.texture;
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
