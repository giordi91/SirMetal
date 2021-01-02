#pragma once

#include "SirMetal/resources/handle.h"
#include <Metal/Metal.h>

namespace SirMetal {
struct EngineContext;
namespace graphics {
struct BlitRequest {
  TextureHandle srcTexture;
  id dstTexture;
  //scissor w, h, x, y
  int scissor[4];
  //viewport w,h,x,y,near, far
  float viewport[6];
  uint32_t customScissor : 1;
  uint32_t customView : 1;
  uint32_t padding : 30;
};
void doBlit(EngineContext *context, id<MTLRenderCommandEncoder> encoder,
            const BlitRequest &request);

}// namespace graphics
}// namespace SirMetal