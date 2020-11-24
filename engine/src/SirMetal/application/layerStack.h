#pragma once

#include <vector>

namespace SirMetal {
struct EngineContext;
class Layer;

// simple wrapper for a vector of layers
class LayerStack final {
public:
  LayerStack() { m_layers.reserve(5); }
  ~LayerStack();

  void pushLayer(Layer *layer, EngineContext *context);
  void popLayer(Layer *layer);
  inline Layer **begin() { return m_layers.data(); }
  inline uint32_t count() const {
    return static_cast<uint32_t>(m_layers.size());
  }

private:
  std::vector<Layer *> m_layers;
};
} // namespace SirMetal
