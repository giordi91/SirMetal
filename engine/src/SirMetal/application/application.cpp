
#include "SirMetal/application/application.h"
#include "SirMetal/application/layer.h"
#include "SirMetal/application/window.h"
#include "SirMetal/core/event.h"
#include "SirMetal/core/input.h"
#include "SirMetal/engine.h"

/*
#include "blackHole/application/layer.h"
#include "blackHole/application/window.h"
#include "blackHole/core/actions.h"
#include "blackHole/core/input.h"
#include "blackHole/core/log.h"
#include "blackHole/engine.h"
#include "blackHole/graphics/camera.h"
#include "blackHole/graphics/debugRenderer.h"
#include "blackHole/graphics/gfxDebug/imguiManager.h"
#include "blackHole/graphics/renderingContext.h"
 */

namespace SirMetal {

Application::Application(const std::string &configFile) {
  // initialize logging

  // TODO add parse config file again
  SirMetal::WindowProps props{"SirMetal", 1280, 720, false, false};
  // EngineConfig engineConfig{"", props, 3};
  EngineConfig engineConfig = loadEngineConfigFile(configFile);

  // create the window with requested config
  m_window = new Window();
  m_window->create(engineConfig.m_windowConfig);
  // this is the function that gets called whenever the window emits an event,
  // for example mouse move etc, it will be calling the application function we
  // can use
  // to handle the events.
  m_window->setEventCallback([this](Event &e) -> void { this->onEvent(e); });

  m_engine = engineStartUp(engineConfig, m_window->getWindow());
  m_engine->m_window = m_window;

  m_window->setInputManagersInWindow(m_engine->m_inputManager);

  // TODO add imgui
  // graphics::initImgui(m_engine);
}

Application::~Application() {
  m_window->destroy();
  engineShutdown(m_engine);
}
void Application::run() {
  while (m_run) {
    m_window->onUpdate();
    m_engine->m_timings.newFrame();
    // TODO process queue event
    // EventQueue *currentQueue = m_queuedEndOfFrameEventsCurrent;
    // flipEndOfFrameQueue();
    // for (uint32_t i = 0; i < currentQueue->allocCount; ++i) {
    //  onEvent(*(currentQueue->events[i]));
    //  delete currentQueue->events[i];
    //}
    // currentQueue->allocCount = 0;
    /*
    m_engine->m_debugRenderer->newFrame();
    m_engine->m_actionManager->processActionButtons();
    m_engine->m_actionManager->processActionAxis();
    m_engine->m_renderingContext->beginScene(0.2f, 0.2f, 0.2f, 1.0f);
    // we can now iterate the layer stack and process the frame
    */
    const int count = m_layerStack.count();
    Layer **layers = m_layerStack.begin();
    @autoreleasepool {
      for (int i = 0; i < count; ++i) {
        layers[i]->onUpdate();
      }
    }
    // m_engine->m_renderingContext->endScene();
    // update input and actions to cache current input for next frame
    m_engine->m_inputManager->swapInputBuffers();
  }
  // lets clean up the layers, now is safe to free up resources
  const int count = m_layerStack.count();
  Layer **layers = m_layerStack.begin();
  for (int i = 0; i < count; ++i) {
    layers[i]->clear();
  }
}
void Application::queueEvent(Event *) const {
  // TODO queueing the frame to be processed either after window update
  // or after the frame
}
void Application::onEvent(Event &e) {
  // The window handles few specific events, other than that it forwards
  // them to the stack starting from top to bottom
  if (e.m_type == EVENT_TYPE::WindowClose) {
    m_run = false;
    return;
  }
  // push events in the layers
  const int count = m_layerStack.count();
  Layer **layers = m_layerStack.begin();
  for (int i = (count - 1); i >= 0; --i) {
    bool eventUsed = layers[i]->onEvent(e);
    if (eventUsed) {
      break;
    }
  }
}

} // namespace SirMetal
