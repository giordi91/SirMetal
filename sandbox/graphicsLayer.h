#pragma once

#include "SirMetal//application/layer.h"
#include "SirMetal/graphics/camera.h"
#include "SirMetal/graphics/debug/frameTimingsWidget.h"
#include "SirMetal/graphics/renderingContext.h"
#include "SirMetal/resources/handle.h"
#include "SirMetal/core/memory/gpu/GPUMemoryAllocator.h"
#import <Metal/Metal.h>

#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

struct DirLight {

  matrix_float4x4 V;
  matrix_float4x4 P;
  matrix_float4x4 VP;
  simd_float4 lightDir;
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
  void generateRandomTexture();

private:
  SirMetal::Camera m_camera;
  SirMetal::FPSCameraController m_cameraController;
  SirMetal::CameraManipulationConfig m_camConfig{};
  SirMetal::EngineContext *m_engine{};
  SirMetal::ConstantBufferHandle m_uniformHandle;
  SirMetal::ConstantBufferHandle m_lightHandle;
  SirMetal::ConstantBufferHandle m_uniforms;
  SirMetal::MeshHandle m_meshes[5];
  SirMetal::LibraryHandle m_shaderHandle;
  SirMetal::LibraryHandle m_rtGenShaderHandle;
  SirMetal::LibraryHandle m_rtShadeShaderHandle;
  SirMetal::LibraryHandle m_rtShadowShaderHandle;
  SirMetal::LibraryHandle m_imageFillHandle;
  SirMetal::LibraryHandle m_fullScreenHandle;
  SirMetal::TextureHandle m_color;
  dispatch_semaphore_t frameBoundarySemaphore;

  SirMetal::graphics::FrameTimingsWidget m_timingsWidget;
  DirLight light{};
  MPSTriangleAccelerationStructure* m_accelerationStructure;
  MPSRayIntersector* m_intersector;
  MPSRayIntersector* m_intersectorShadow;
  SirMetal::BufferHandle m_tBuff;
  SirMetal::BufferHandle m_rayBuffer[2]{};
  SirMetal::BufferHandle m_intersectionBuffer;
  SirMetal::GPUMemoryAllocator m_gpuAllocator;
  id rayPipeline;
  id rayShadePipeline;
  id testFillPipeline;

  id _randomTexture;

};
} // namespace Sandbox
