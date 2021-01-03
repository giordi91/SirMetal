#pragma once
#include "SirMetal/graphics/metalBvh.h"
#include "SirMetal/resources/handle.h"

#include <vector>

#define RT 1

struct TexRect {
  int x, y, w, h;
};
struct PackingResult {
  std::vector<TexRect> rectangles;
  int w;
  int h;
};

namespace SirMetal {
struct EngineContext;
struct GLTFAsset;

namespace graphics {

class LightMapper {
  public:
  void initialize(EngineContext *context, const char *gbufferShader,const char *gbufferClearShader,
                             const char *rtShader);

  void setAssetData(EngineContext *context, GLTFAsset *asset, int individualLightMapSize);
  [[nodiscard]] const PackingResult &getPackResult() const { return packResult; }
  void bakeNextSample(EngineContext *context, id<MTLCommandBuffer> commandBuffer,
                      ConstantBufferHandle uniforms, id randomTexture);

  private:
  void recordRtArgBuffer(EngineContext *context, GLTFAsset *asset);
  void recordRasterArgBuffer(EngineContext *context, GLTFAsset *asset);
  void allocateTextures(EngineContext *context, int w, int h);
  PackingResult buildPacking(int maxSize, int individualSize, int count);
  void doGBufferPass(EngineContext *context, id<MTLCommandBuffer> commandBuffer);
  void doLightMapBake(EngineContext *context, id<MTLCommandBuffer> commandBuffer,
                      ConstantBufferHandle uniforms, id randomTexture);

  public:
  TextureHandle m_gbuff[2];
  TextureHandle m_lightMap;
  int rtSampleCounter = 0;
  int requestedSamples = 400;
  uint32_t rtFrameCounterFull = 0;

  private:
  id rtLightmapPipeline;
  GLTFAsset *m_asset;
  int m_lightMapSize;
  PackingResult packResult;
  LibraryHandle m_rtLightMapHandle;
  LibraryHandle m_gbuffHandle;
  LibraryHandle m_gbuffClearHandle;
  id argRtBuffer;
  id argBuffer;
  id argBufferFrag;
  MetalBVH accelStruct;
};


}// namespace graphics
}// namespace SirMetal