#pragma once

#include "SirMetal//application/layer.h"
#include "SirMetal/graphics/camera.h"
#include "SirMetal/resources/handle.h"
#import <Metal/Metal.h>

namespace SirMetal {
struct EngineContext;

} // namespace SirMetal
namespace Sandbox {
class GraphicsLayer final : public SirMetal::Layer {
public:
  GraphicsLayer() : Layer("Sandbox") {}
  ~GraphicsLayer() override = default;
  void onAttach(SirMetal::EngineContext *context) override;
  void onDetach() override;
  void onUpdate() override;
  bool onEvent(SirMetal::Event &event) override;
  void clear() override;

private:
  void updateUniformsForView(float screenWidth, float screenHeight);
  void makePipeline();
  void makeBuffers();

private:
  SirMetal::Camera m_camera;
  SirMetal::FPSCameraController m_cameraController;
  SirMetal::CameraManipulationConfig m_camConfig{};
  SirMetal::EngineContext *m_engine{};
  MTLClearColor color;
  SirMetal::ConstantBufferHandle m_uniformHandle;
  SirMetal::MeshHandle m_mesh;
  id<MTLDepthStencilState> m_depthState;
  SirMetal::TextureHandle m_depthHandle;
};
} // namespace Sandbox