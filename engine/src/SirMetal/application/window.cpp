#include "window.h"

#include <assert.h>

#import <QuartzCore/CAMetalLayer.h>

#include "SirMetal/core/input.h"
#include "SirMetal/engine.h"

//#include "blackHole/graphics/gfxDebug/imgui_impl_sdl.h"

namespace SirMetal {
bool Window::create(const WindowProps &props) {

  SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
  SDL_InitSubSystem(SDL_INIT_VIDEO);
  m_window = SDL_CreateWindow(props.m_title.c_str(), -1, -1, props.m_width,
                              props.m_height, SDL_WINDOW_ALLOW_HIGHDPI);
  SDL_SetWindowResizable(m_window, SDL_TRUE);

  // Check that the window was successfully created
  if (m_window == nullptr) {
    // In the case that the window could not be made...
    printf("[Error] Could not create window: %s\n", SDL_GetError());
    return false;
  }

  return true;
}

void Window::destroy() {
  // Close and destroy the window
  SDL_DestroyWindow(m_window);

  // Clean up
  SDL_Quit();
}

void handleEvent(const SDL_Event event, SirMetal::Input *input) {
  // ImGui_ImplSDL2_ProcessEvent(&event);
  // static SDL_Joystick* joy = SDL_JoystickOpen(0);
  switch (event.type) {
  case SDL_WINDOWEVENT: {
    switch (event.window.event) {
    case SDL_WINDOWEVENT_RESIZED: {
      // printf("window resized {} {} {} ", event.window.windowID,
      //             event.window.data1, event.window.data2);
      break;
    }
    }
    break;
  }
  case SDL_KEYDOWN: {
    input->keyDown(event.key.keysym.scancode);
    break;
  }

  case SDL_KEYUP: {
    input->keyUp(event.key.keysym.scancode);
    break;
  }

  case SDL_MOUSEMOTION: {
    input->mouse.position.x = event.motion.x;
    input->mouse.position.y = event.motion.y;
    input->mouse.position.xRel = event.motion.xrel;
    input->mouse.position.yRel = event.motion.yrel;
    break;
  }

  case SDL_MOUSEBUTTONDOWN: {
    input->mouseButtonDown(event.button.button);
    break;
  }

  case SDL_MOUSEBUTTONUP: {
    input->mouseButtonUp(event.button.button);
    break;
  }
  }
}

void Window::onUpdate() const {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      Event e;
      e.m_type = EVENT_TYPE::WindowClose;
      e.m_category = EVENT_CATEGORY::EventCategoryApplication;
      assert(m_callback != nullptr);
      m_callback(e);
    }
    handleEvent(event, m_inputManager);
  }
}

} // namespace SirMetal
