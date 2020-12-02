#pragma once

#import <objc/objc.h>
#import <Metal/Metal.h>
namespace SirMetal {
struct EngineContext;
}
namespace SirMetal::graphics {
void initImgui(EngineContext *context);
void imguiNewFrame(EngineContext *context,
                   MTLRenderPassDescriptor *renderPassDescriptor);
void imguiEndFrame(id<MTLCommandBuffer> commandBuffer,
                   id<MTLRenderCommandEncoder> commandEncoder);
void shutdownImgui();

} // namespace SirMetal::graphics