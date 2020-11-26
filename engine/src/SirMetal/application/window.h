#pragma once

#import <QuartzCore/CAMetalLayer.h>
#include <functional>

#include "SDL.h"

#include "SirMetal/core/event.h"

namespace SirMetal {
struct WindowProps;

class Input;

class Window final {
public:
  // type alias for the function to call when we receive an event
  using EventCallbackFn = std::function<void(Event &)>;

public:
  bool create(const WindowProps &props);
  void destroy();

  // HWND getHwnd() const { return m_handle; }
  void onUpdate() const;
  void setEventCallback(const EventCallbackFn &callback) {
    m_callback = callback;
  }
  SDL_Window* getWindow(){ return m_window;}

  void setInputManagersInWindow(Input *input) { m_inputManager = input; }

private:
  SDL_Window *m_window{};
  EventCallbackFn m_callback{};
  Input *m_inputManager{};
};

} // namespace SirMetal