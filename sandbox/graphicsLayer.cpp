#include "graphicsLayer.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "SirMetal/application/window.h"
#include "SirMetal/engine.h"
#include "SirMetal/core/input.h"



/*
#include "blackHole/application/application.h"
#include "blackHole/application/window.h"
#include "blackHole/core/actions.h"
#include "blackHole/core/log.h"
#include "blackHole/engine.h"
#include "blackHole/graphics/buffermanager.h"
#include "blackHole/graphics/debugRenderer.h"
#include "blackHole/graphics/meshManager.h"
#include "blackHole/graphics/renderingContext.h"
#include "blackHole/graphics/shaderManager.h"
#include "blackHole/graphics/textureManager.h"

// Temporary data
BlackHole::ShaderHandle VSMESH;
BlackHole::ShaderHandle PSMESH;
BlackHole::BufferHandle MATRIX_BUFFER;
BlackHole::MeshHandle REAL_MESH;
BlackHole::TextureHandle TEX;
 */

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
}

void GraphicsLayer::onDetach() {}

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
  }
}

bool GraphicsLayer::onEvent(SirMetal::Event &) {
  // TODO start processing events the game cares about, like
  // etc...
  return false;
}

void GraphicsLayer::clear() {}
} // namespace Sandbox
