#pragma once

#include "SirMetal//application/layer.h"
#include "SirMetal/core/memory/gpu/GPUMemoryAllocator.h"
#include "SirMetal/graphics/camera.h"
#include "SirMetal/graphics/debug/frameTimingsWidget.h"
#include "SirMetal/graphics/debug/gpuWidget.h"
#include "SirMetal/graphics/renderingContext.h"
#include "SirMetal/resources/gltfLoader.h"
#include "SirMetal/resources/handle.h"
#import <Metal/Metal.h>
#include <SirMetal/graphics/PSOGenerator.h>

#define RT 1
struct DirLight {

  matrix_float4x4 V;
  matrix_float4x4 P;
  matrix_float4x4 VP;
  simd_float4 lightDir;
};

struct TexRect
{
  int x,y,w,h;
};

struct PackingResult
{
  int w;
  int h;
  std::vector<TexRect> rectangles;

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
  bool updateUniformsForView(float screenWidth, float screenHeight,uint32_t lightMapSize);
  void updateLightData();
  void renderDebugWindow();
  void generateRandomTexture(uint32_t w, uint32_t h);
  void recordRasterArgBuffer();

#if RT
  void recordRTArgBuffer();
  void buildAccellerationStructure();
  id buildPrimitiveAccelerationStructure(MTLAccelerationStructureDescriptor *descriptor);
#endif

  private:
  SirMetal::Camera m_camera;
  SirMetal::FPSCameraController m_cameraController;
  SirMetal::CameraManipulationConfig m_camConfig{};
  SirMetal::EngineContext *m_engine{};
  SirMetal::ConstantBufferHandle m_camUniformHandle;
  SirMetal::ConstantBufferHandle m_lightHandle;
  SirMetal::ConstantBufferHandle m_uniforms;
  SirMetal::LibraryHandle m_shaderHandle;
  SirMetal::LibraryHandle m_gbuffHandle;
  SirMetal::LibraryHandle m_fullScreenHandle;
  SirMetal::LibraryHandle m_rtLightMap;
  SirMetal::TextureHandle m_color[2];
  SirMetal::TextureHandle m_gbuff[3];
  SirMetal::TextureHandle m_lightMap;
  SirMetal::TextureHandle m_depthHandle;
  dispatch_semaphore_t frameBoundarySemaphore;

  SirMetal::graphics::FrameTimingsWidget m_timingsWidget;
  SirMetal::graphics::GPUInfoWidget m_gpuInfo;
  DirLight light{};
  SirMetal::GPUMemoryAllocator m_gpuAllocator;
  id rtLightmapPipeline;

  id m_randomTexture;

  id argRtBuffer;
  id argBuffer;
  id argBufferFrag;
  id sampler;

  std::vector<id> primitiveAccelerationStructures;
  id<MTLBuffer> instanceBuffer;
  id instanceAccelerationStructure;

  SirMetal::GLTFAsset asset;
  uint32_t rtFrameCounter = 0;
  uint32_t rtFrameCounterFull = 0;
  uint32_t lightMapSize = 1024;
  PackingResult packResult;
  void allocateGBufferTexture(int w,int h);
  void doGBufferPass(id<MTLCommandBuffer> commandBuffer, int index);
  void doLightmapBake(id<MTLCommandBuffer> buffer, int index);
  void doRasterRender(id<MTLRenderCommandEncoder> commandEncoder,
                      const SirMetal::PSOCache &cache);
  PackingResult buildPacking(int maxSize, int individualSize, int count);
};
}// namespace Sandbox
