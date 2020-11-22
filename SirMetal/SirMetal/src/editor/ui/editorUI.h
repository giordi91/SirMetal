#pragma once

#import "imgui.h"

namespace SirMetal {
namespace Editor {
class EditorUI {

 public:
  void beginUI(MTKView* view, MTLRenderPassDescriptor* passDescriptor);
  void endUI(id<MTLCommandBuffer> commandBuffer,id<MTLRenderCommandEncoder> encoder);
  void show(int width, int height);
};
}
}


