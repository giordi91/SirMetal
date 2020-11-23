#pragma once

#import <objc/objc.h>
@class MTKView;
@protocol MTLCommandBuffer;
@class MTLRenderPassDescriptor;
@protocol MTLRenderCommandEncoder;

namespace SirMetal {
namespace Editor {
class ImguiRenderer {

 public:
  void initialize(id<MTLDevice> device);
  void beginUI(MTKView* view, MTLRenderPassDescriptor* passDescriptor);
  void endUI(id<MTLCommandBuffer> commandBuffer,id<MTLRenderCommandEncoder> encoder);
  void show(int width, int height);
};
}
}


