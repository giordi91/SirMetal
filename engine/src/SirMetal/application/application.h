#pragma once

#include <string>

#include "SirMetal/application/layerStack.h"

namespace SirMetal {

class Window;
struct EngineContext;
struct Event;

class Application {
public:
  Application(const std::string &configFile);
  virtual ~Application();
  void run();

  void onEvent(Event &e);
  void queueEvent(Event *e) const;

protected:
  Window *m_window;
  bool m_run = true;
  LayerStack m_layerStack;
  EngineContext *m_engine;
};

} // namespace SirMetal
