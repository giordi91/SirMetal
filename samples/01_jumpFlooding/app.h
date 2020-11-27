#pragma once

#include "SirMetal//application/application.h"
#include "graphicsLayer.h"

char const *ENGINE_CONFIG_FILE = "config.json";
class JumpFloodApplication final : public SirMetal::Application {
public:
  JumpFloodApplication() : Application(ENGINE_CONFIG_FILE) {
    // the heavy lifting of the initialization happens in Application() not sure
    // how much I like this, I would rather not have much work in a constructor.
    // Either way, we create a GraphicsLayer which contains most of the game
    // logic and we push it on the stack. The stack gets ownership of that
    auto *graphicsLayer = new GraphicsLayer();
    m_layerStack.pushLayer(graphicsLayer, m_engine);
  }
};
