#import <MetalKit/MetalKit.h>

#import "editorUI.h"
#import "imgui_internal.h"
#import "imgui_impl_osx.h"
#import "imgui_impl_metal.h"

namespace SirMetal {
namespace Editor {

void EditorUI::show(int width, int height) {

  ImGui::ShowDemoWindow((bool*)0);

}
void EditorUI::beginUI(MTKView* view, MTLRenderPassDescriptor* passDescriptor) {
  // Start the Dear ImGui frame
  ImGui_ImplOSX_NewFrame(view);
  ImGui_ImplMetal_NewFrame(passDescriptor);
  ImGui::NewFrame();

}
void EditorUI::endUI(id<MTLCommandBuffer> commandBuffer,id<MTLRenderCommandEncoder> encoder) {
  // Rendering
  ImGui::Render();
  ImDrawData *drawData = ImGui::GetDrawData();
  ImGui_ImplMetal_RenderDrawData(drawData,  commandBuffer, encoder);

}

}
}
