#pragma once

#include "SirMetal//application/layer.h"
#import <Metal/Metal.h>
#include "SirMetal/graphics/camera.h"

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
  void updateUniformsForView(float screenWidth ,float screenHeight);
  void makePipeline();
  void makeBuffers();

private:
  SirMetal::Camera m_camera;
  SirMetal::FPSCameraController m_cameraController;
  SirMetal::CameraManipulationConfig m_camConfig{};
  SirMetal::EngineContext* m_engine{};
  MTLClearColor color;

};
}  // namespace Game