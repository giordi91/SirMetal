#pragma once

#include "SirMetal//application/layer.h"
#import <Metal/Metal.h>
//#include "blackHole/graphics/camera.h"

namespace SirMetal{
struct EngineContext;

}  // namespace BlackHole
namespace Sandbox{
class GraphicsLayer final : public SirMetal::Layer {
public:
  GraphicsLayer() : Layer("Sandbox") {}
  ~GraphicsLayer() override = default;
  void onAttach(SirMetal::EngineContext* context) override;
  void onDetach() override;
  void onUpdate() override;
  bool onEvent(SirMetal::Event& event) override;
  void clear() override;

private:
  //BlackHole::Camera3DPivot camera;
  SirMetal::EngineContext* m_engine{};
  MTLClearColor color;

};
}  // namespace Game