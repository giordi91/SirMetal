
#include <assert.h>

#include "SirMetal/application/layerStack.h"
#include "SirMetal/application/layer.h"

namespace SirMetal {
LayerStack::~LayerStack() {
  for (Layer *l : m_layers) {
    l->onDetach();
    delete l;
  }
}
void LayerStack::pushLayer(Layer *layer, EngineContext *context) {
  m_layers.push_back(layer);
  layer->onAttach(context);
}

void LayerStack::popLayer(Layer *) {
  assert(0 && "layer pop is not implemented yet");
}
} // namespace SirMetal
