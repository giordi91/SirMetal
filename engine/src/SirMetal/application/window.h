#pragma once

#import <QuartzCore/CAMetalLayer.h>
#include <functional>

#include "SDL.h"

#include "SirMetal/core/event.h"

namespace SirMetal {
struct WindowProps;

class Input;

class ActionManager;

class SDLWindow final {
public:
  // type alias for the function to call when we receive an event
  using EventCallbackFn = std::function<void(Event&)>;

public:
  bool create(const WindowProps &props);
  void destory();

  // HWND getHwnd() const { return m_handle; }
  void onUpdate() const;
   void setEventCallback(const EventCallbackFn& callback) {
    m_callback = callback;
  }

  void setInputManagersInWindow(Input *input) {
    m_inputManager = input;
  }

  SDL_Window *getWindow() const { return m_window; }
  id getGPU() const { return gpu; }
  id getQueue() const { return queue; }
  id getSwapchain() const { return swapchain; };

private:
  SDL_Window *m_window{};
  id gpu;
  id queue;
  // HWND m_handle{};
  EventCallbackFn m_callback{};
  Input *m_inputManager{};
  SDL_Renderer *m_renderer;
  CAMetalLayer *swapchain;
};

} // namespace SirMetal