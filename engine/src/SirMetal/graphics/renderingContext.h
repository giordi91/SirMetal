#pragma once

#import <QuartzCore/CAMetalLayer.h>
#include <objc/objc.h>

struct SDL_Window;
struct SDL_Renderer;

namespace SirMetal {
struct EngineConfig;
} // namespace SirMetal

namespace SirMetal::graphics {

struct DrawTracker
{
  id renderTargets[8];
  id depthTarget;
};


class RenderingContext {
public:
  RenderingContext() = default;
  ~RenderingContext() = default;
  // not copyable/movable etc
  RenderingContext(const RenderingContext &) = delete;
  RenderingContext &operator=(const RenderingContext &) = delete;
  RenderingContext(RenderingContext &&) = delete;
  RenderingContext &operator=(RenderingContext &&) = delete;

  bool initialize(const EngineConfig &config,SDL_Window *window);
  void cleanup() const;

  inline id getDevice() const { return m_gpu; }
  inline id getQueue ()const {return m_queue;}
  inline CAMetalLayer* getSwapchain()const {return m_swapchain;}

  inline void beginScene(const float r, const float g, const float b,
                         const float alpha) const {
    /*
    float color[4];

    // Setup the color to clear the buffer to.
    color[0] = r;
    color[1] = g;
    color[2] = b;
    color[3] = alpha;

    // Clear the back buffer.
    m_adapterResult.m_deviceContext->ClearRenderTargetView(m_renderTargetView,
                                                           color);

    // Clear the depth buffer.
    m_adapterResult.m_deviceContext->ClearDepthStencilView(
        m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
        */
  }
  inline void endScene() const {
    /*
    // Present the back buffer to the screen since rendering is complete.
    // if (m_vsync_enabled) {
    // Lock to screen refresh rate.
    m_swapChain->Present(1, 0);
    //} else {
    //  // Present as fast as possible.
    //  m_swapChain->Present(0, 0);
    //}
     */
  }

  void flush();

private:
  id m_gpu;
  id m_queue;
  CAMetalLayer *m_swapchain;
  SDL_Renderer *m_renderer;
};
} // namespace SirMetal::graphics
