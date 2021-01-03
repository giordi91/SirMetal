#pragma once

#include "SirMetal//application/layer.h"
#include "SirMetal/core/memory/gpu/GPUMemoryAllocator.h"
#include "SirMetal/graphics/camera.h"
#include "SirMetal/graphics/renderingContext.h"
#include "SirMetal/resources/gltfLoader.h"
#include "SirMetal/resources/handle.h"
#include "SirMetal/graphics/PSOGenerator.h"
#include "SirMetal/graphics/metalBvh.h"
#include "SirMetal/graphics/debug/frameTimingsWidget.h"
#include "SirMetal/graphics/debug/gpuWidget.h"
#include "SirMetal/graphics/lightmapper.h"

#define RT 1
struct DirLight {

  matrix_float4x4 V;
  matrix_float4x4 P;
  matrix_float4x4 VP;
  simd_float4 lightDir;
};

namespace SirMetal {
struct EngineContext;

}// namespace SirMetal
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
  bool updateUniformsForView(float screenWidth, float screenHeight,
                             uint32_t lightMapSize);
  void renderDebugWindow();
  void generateRandomTexture(uint32_t w, uint32_t h);
  void recordRasterArgBuffer();
  void doRasterRender(id<MTLRenderCommandEncoder> commandEncoder,
                      const SirMetal::PSOCache &cache);

  private:
  SirMetal::Camera m_camera;
  SirMetal::FPSCameraController m_cameraController;
  SirMetal::CameraManipulationConfig m_camConfig{};
  SirMetal::EngineContext *m_engine{};
  SirMetal::ConstantBufferHandle m_camUniformHandle;
  SirMetal::ConstantBufferHandle m_uniforms;
  SirMetal::LibraryHandle m_shaderHandle;
  SirMetal::LibraryHandle m_fullScreenHandle;
  SirMetal::TextureHandle m_depthHandle;
  dispatch_semaphore_t frameBoundarySemaphore;

  SirMetal::graphics::FrameTimingsWidget m_timingsWidget;
  SirMetal::graphics::GPUInfoWidget m_gpuInfo;

  id m_randomTexture;

  id argBuffer;
  id argBufferFrag;

  SirMetal::GLTFAsset asset;

  uint32_t lightMapSize = 1024;
  bool debugFullScreen = false;
  int currentDebug = 0;
  SirMetal::graphics::LightMapper m_lightMapper;
};
}// namespace Sandbox
