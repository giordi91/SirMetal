#import <MetalKit/MetalKit.h>

#import "imguiRenderer.h"
#import "imgui/imgui.h"
#import "imgui/imgui_internal.h"
#import "imgui/imgui_impl_osx.h"
#import "imgui/imgui_impl_metal.h"

namespace SirMetal {
namespace Editor {

void ImguiRenderer::show(int width, int height) {

  ImGui::ShowDemoWindow((bool*)0);

}
void ImguiRenderer::beginUI(MTKView* view, MTLRenderPassDescriptor* passDescriptor) {
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize.x = static_cast<float>(view.bounds.size.width);
  io.DisplaySize.y = static_cast<float>(view.bounds.size.height);

  CGFloat framebufferScale = view.window.screen.backingScaleFactor ?: NSScreen.mainScreen.backingScaleFactor;
  io.DisplayFramebufferScale = ImVec2(framebufferScale, framebufferScale);
  io.DeltaTime = 1 / float(view.preferredFramesPerSecond ?: 60);

  // Start the Dear ImGui frame
  ImGui_ImplOSX_NewFrame(view);
  ImGui_ImplMetal_NewFrame(passDescriptor);
  ImGui::NewFrame();

}
void ImguiRenderer::endUI(id<MTLCommandBuffer> commandBuffer, id<MTLRenderCommandEncoder> encoder) {
  // Rendering
  ImGui::Render();
  ImDrawData *drawData = ImGui::GetDrawData();
  ImGui_ImplMetal_RenderDrawData(drawData,  commandBuffer, encoder);

}
void ImguiRenderer::initialize(id<MTLDevice> device) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGui_ImplMetal_Init(device);

}

}
}
