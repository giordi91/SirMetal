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

//basic lightmapping implementation, a lot more work is needed to make it actually "production ready" or usable
//for a game, but the overal plumbing is done.
class LightMapper {
  public:
  //shaders are loaded at initialization time, this is because you might want to pass different
  //shaders to perform the calculation, especially in the lightmapping stage.
  void initialize(EngineContext *context, const char *gbufferShader,
                  const char *gbufferClearShader, const char *rtShader);
  //here is when all the heavy lighting of the accelleration structure happens
  void setAssetData(EngineContext *context, GLTFAsset *asset, int individualLightMapSize);
  //the packing result contains where the different lightmap ended up being in the atlas
  [[nodiscard]] const PackingResult &getPackResult() const { return m_packResult; }
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
  //how many samples have actually been done
  int m_rtSampleCounter = 0;
  int m_requestedSamples = 400;
  //this value records the number of frames passed since we started baking
  uint32_t m_rtFrameCounterFull = 0;

  private:
  id m_rtLightmapPipeline;
  GLTFAsset *m_asset;
  int m_lightMapSize;
  PackingResult m_packResult;
  LibraryHandle m_rtLightMapHandle;
  LibraryHandle m_gbuffHandle;
  LibraryHandle m_gbuffClearHandle;
  //TODO this can probably be optimized and do not need a dedicated RT arg buffer
  //we can reuse both vertex and fragment argument buffers
  id m_argRtBuffer;
  id m_argBuffer;
  MetalBVH m_accelStruct;
};


}// namespace graphics
}// namespace SirMetal