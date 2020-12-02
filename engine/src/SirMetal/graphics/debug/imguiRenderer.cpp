#include "SirMetal/graphics/debug/imguiRenderer.h"
#include "SirMetal/application/window.h"
#include "SirMetal/engine.h"
#include "SirMetal/graphics/debug/imgui/imgui.h"
#include "SirMetal/graphics/debug/imgui/imgui_impl_metal.h"
#include "SirMetal/graphics/debug/imgui/imgui_impl_sdl.h"
#include "SirMetal/graphics/renderingContext.h"

namespace SirMetal::graphics {

void initImgui(EngineContext *context) {
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable
  // Keyboard Controls io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; //
  // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForMetal(context->m_window->getWindow());
  ImGui_ImplMetal_Init(context->m_renderingContext->getDevice());
}

void imguiNewFrame(EngineContext *context,
                   MTLRenderPassDescriptor *renderPassDescriptor) {
  ImGui_ImplMetal_NewFrame(renderPassDescriptor);
  ImGui_ImplSDL2_NewFrame(context->m_window->getWindow());
  ImGui::NewFrame();
}

void imguiEndFrame(id<MTLCommandBuffer> commandBuffer,
                   id<MTLRenderCommandEncoder> commandEncoder) {
  ImGui::Render();
  ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer,
                                 commandEncoder);
}

void shutdownImgui() {
  ImGui_ImplMetal_Shutdown();
  ImGui_ImplSDL2_Shutdown();
}
} // namespace SirMetal::graphics