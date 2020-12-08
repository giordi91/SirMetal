#include "SirMetal/graphics/renderingContext.h"

#include "SirMetal/engine.h"
#import <Metal/Metal.h>
#include <SDL_render.h>

namespace SirMetal::graphics {

bool RenderingContext::initialize(const EngineConfig &config,
                                  SDL_Window *window) {

  m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
  m_swapchain = (__bridge CAMetalLayer *)SDL_RenderGetMetalLayer(m_renderer);
  //m_swapchain.displaySyncEnabled = false;
  m_swapchain.device = MTLCreateSystemDefaultDevice();
  m_queue = [m_swapchain.device newCommandQueue];
  m_gpu = m_swapchain.device;

  return true;
}

void RenderingContext::cleanup() const { SDL_DestroyRenderer(m_renderer); }
void RenderingContext::flush() {


  id frameBoundarySemaphore = dispatch_semaphore_create(0);
  id<MTLCommandBuffer> commandBuffer = [m_queue commandBuffer];
  __block dispatch_semaphore_t block_semaphore = frameBoundarySemaphore;
  [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer) {
    // GPU work is complete
    // Signal the semaphore to start the CPU work
    dispatch_semaphore_signal(block_semaphore);
  }];
  [commandBuffer commit];

  //wait semaphore
  dispatch_semaphore_wait(block_semaphore, DISPATCH_TIME_FOREVER);
}
} // namespace SirMetal::graphics
