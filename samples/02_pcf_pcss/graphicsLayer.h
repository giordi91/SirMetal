#pragma once

#include "SirMetal//application/layer.h"
#include "SirMetal/graphics/camera.h"
#include "SirMetal/graphics/debug/frameTimingsWidget.h"
#include "SirMetal/graphics/renderingContext.h"
#include "SirMetal/resources/handle.h"
#import <Metal/Metal.h>

struct DirLight {

  matrix_float4x4 V;
  matrix_float4x4 P;
  matrix_float4x4 VP;
  simd_float4 lightDir;
  float lightSize;
  float near;
  float pcfsize;
  int pcfsamples;
  int blockerCount;
  int showBlocker;
  int algType;
};
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
  void updateLightData();
  void renderDebugWindow();

private:
  SirMetal::Camera m_camera;
  SirMetal::FPSCameraController m_cameraController;
  SirMetal::CameraManipulationConfig m_camConfig{};
  SirMetal::EngineContext *m_engine{};
  SirMetal::ConstantBufferHandle m_camUniformHandle;
  SirMetal::ConstantBufferHandle m_lightHandle;
  SirMetal::MeshHandle m_meshes[5];
  SirMetal::TextureHandle m_depthHandle;
  SirMetal::TextureHandle m_shadowHandle;
  SirMetal::LibraryHandle m_shadowShaderHandle;
  SirMetal::LibraryHandle m_shaderHandle;
  dispatch_semaphore_t frameBoundarySemaphore;

  SirMetal::graphics::FrameTimingsWidget m_timingsWidget;
  DirLight light{};
  int m_shadowAlgorithmsLast = 0;
};
} // namespace Sandbox
