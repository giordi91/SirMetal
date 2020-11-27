
#include "sandbox.h"
#import <Metal/Metal.h>

#import <QuartzCore/CAMetalLayer.h>
#include <SDL.h>

#include "SirMetal/application/window.h"
#import "SirMetal/engine.h"
#include <iostream>

int main(int argc, char *args[]) {
  std::cout<<args[0]<<std::endl;
  Sandbox::SandboxApplication sand;
  sand.run();
  /*
  SirMetal::Window sdlWin;
  SirMetal::WindowProps props{"SirMetal", 1280, 720, false, false};
  sdlWin.create(props);

  // temporary to get the window to work
  CAMetalLayer *swapchain = sdlWin.getSwapchain();
  id<MTLCommandQueue> queue = sdlWin.getQueue();
  MTLClearColor color = MTLClearColorMake(0, 0, 0, 1);
  bool quit = false;
  SDL_Event e;

  while (!quit) {
    while (SDL_PollEvent(&e) != 0) {
      switch (e.type) {
      case SDL_QUIT:
        quit = true;
        break;
      }
    }

    //@autoreleasepool {
    id<CAMetalDrawable> surface = [swapchain nextDrawable];

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
    //}
  }
  sdlWin.destroy();
   */

  return 0;
}

/*
#include <assert.h>
#include <stdio.h>

#include <iostream>

#define SDL_MAIN_HANDLED
#include <SDL_syswm.h>

#include "SDL.h"

void handleEvent(const SDL_Event event) {
  switch (event.type) {
    case SDL_WINDOWEVENT: {
      switch (event.window.event) {
        case SDL_WINDOWEVENT_RESIZED: {
          std::cout << "window resized " << event.window.windowID << " "
                    << event.window.data1 << " " << event.window.data2
                    << std::endl;
          break;
        }
      }
      break;
    }
    case SDL_JOYAXISMOTION:
    {
      // Analog joystick dead zone
      constexpr int joystickDeadZone = 8000;
      if ((event.jaxis.value < -joystickDeadZone) |
          (event.jaxis.value > joystickDeadZone)) {
        if (event.jaxis.axis == 0) {
          std::cout << "left/right " << event.jaxis.value << std::endl;
        }

        if (event.jaxis.axis == 1) {
          std::cout << "up/down " << event.jaxis.value << std::endl;
        }
      }
      break;
    }
    case SDL_JOYBUTTONDOWN:
    {
      std::cout << "controller button "
                << static_cast<int>(event.jbutton.button) << std::endl;
      break;
    }
  }
}

int main() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
    fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
    exit(1);
  }

  // C++17 allows us to use the attribute maybe_unused, we are using the result
  // only in debug for doing the assert, as is in release we would have an
  // unused variable and the compiler would be yelling at us, with this
  // attribute the compiler is free to ignore the unused value as such we would
  // get no warn in release
  [[maybe_unused]] auto result = SDL_SetHintWithPriority(
      SDL_HINT_RENDER_DRIVER, "direct3d11", SDL_HINT_OVERRIDE);
  assert(result == SDL_TRUE);

  // Create an application window with the following settings:
  SDL_Window* window =
      SDL_CreateWindow("An SDL2 window",         // window title
                       SDL_WINDOWPOS_UNDEFINED,  // initial x position
                       SDL_WINDOWPOS_UNDEFINED,  // initial y position
                       640,                      // width, in pixels
                       480,                      // height, in pixels
                       0                         // flags - see below
      );
  SDL_SetWindowResizable(window, SDL_TRUE);

  // Check that the window was successfully created
  if (window == nullptr) {
    // In the case that the window could not be made...
    printf("Could not create window: %s\n", SDL_GetError());
    return 1;
  }

  // extracting Windows window handle that we will use to initialize the
  // swapchain
  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version)
  SDL_GetWindowWMInfo(window, &wmInfo);

  SDL_JoystickEventState(SDL_ENABLE);
  SDL_Joystick* joystick = SDL_JoystickOpen(0);

  static float r = 0, g = 0, b = 0;
  constexpr float deltaColor = 0.01f;
  while (true) {
    // Get the next event
    SDL_Event event;
    if (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        break;
      }
      handleEvent(event);
    }
  }

  // Close and destroy the window
  SDL_DestroyWindow(window);

  // Clean up
  SDL_Quit();
  return 0;
}
*/
