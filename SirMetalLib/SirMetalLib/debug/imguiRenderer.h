#pragma once

#import "imgui/imgui.h"

namespace SirMetal {
namespace Editor {
class ImguiRenderer {

 public:
  void beginUI(MTKView* view, MTLRenderPassDescriptor* passDescriptor);
  void endUI(id<MTLCommandBuffer> commandBuffer,id<MTLRenderCommandEncoder> encoder);
  void show(int width, int height);
};
}
}


