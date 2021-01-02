#pragma once
#include "SirMetal/resources/handle.h"
#include "SirMetal/graphics/metalBvh.h"

#include <vector>

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
  void initialize(EngineContext *context, const char *gbufferShader,
                  const char *rtShader);

  void setAssetData(EngineContext* context, GLTFAsset* asset,int individualLightMapSize);
  void recordRtArgBuffer(EngineContext *context, GLTFAsset *asset);
  void recordRasterArgBuffer(EngineContext*context, GLTFAsset* asset);
  void allocateTextures(EngineContext* context, int w ,int h);
  PackingResult buildPacking(int maxSize, int individualSize, int count);
  void doGBufferPass(EngineContext* context, id<MTLCommandBuffer> commandBuffer);

  LibraryHandle m_rtLightMapHandle;
  LibraryHandle m_gbuffHandle;
  id argRtBuffer;
  id argBuffer;
  id argBufferFrag;
  MetalBVH accelStruct;
  TextureHandle m_gbuff[3];
  TextureHandle m_lightMap;
  PackingResult packResult;
  GLTFAsset* m_asset;
  int m_lightMapSize;
};


}// namespace graphics
}// namespace SirMetal