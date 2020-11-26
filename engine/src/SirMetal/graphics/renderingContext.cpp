#include "SirMetal/graphics/renderingContext.h"

#include <SDL_render.h>
#include "SirMetal/engine.h"

namespace SirMetal::graphics {

bool RenderingContext::initialize(const EngineConfig &config,SDL_Window *window) {

  m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
  m_swapchain = (__bridge CAMetalLayer *)SDL_RenderGetMetalLayer(m_renderer);
  m_queue = [m_swapchain.device newCommandQueue];
  m_gpu = m_swapchain.device;

  return true;
}

void RenderingContext::cleanup() const {
  SDL_DestroyRenderer(m_renderer);
}
} // namespace SirMetal::graphics
