#include "SirMetal/graphics/debug/gpuWidget.h"
#include "SirMetal/graphics/debug/imgui/imgui.h"
#include "SirMetal/core/core.h"

#include <Metal/Metal.h>

namespace SirMetal::graphics{

void GPUInfoWidget::render(id device) {
  id<MTLDevice> actualDevice = device;

  if(ImGui::CollapsingHeader("GPU Info")) {
    ImGui::Text("Name: %s", actualDevice.name.UTF8String);
    auto memorySize = static_cast<uint32_t>(actualDevice.recommendedMaxWorkingSetSize * BYTE_TO_MB_D);
    ImGui::Text("VRAM: %uMB", memorySize);
    bool unifiedMemory = actualDevice.hasUnifiedMemory;
    ImGui::Checkbox("Unified memory",&unifiedMemory);
    bool barycentric = actualDevice.areBarycentricCoordsSupported;
    ImGui::Checkbox("Barycentric",&barycentric);
    auto argTier = actualDevice.argumentBuffersSupport;
    ImGui::Text("Argument buffers: %s",argTier == MTLArgumentBuffersTier2 ? "Tier2" : "Tier1");
    ImGui::Text ("Max Arg samplers: %d",static_cast<int>(actualDevice.maxArgumentBufferSamplerCount));
    bool bc= actualDevice.supportsBCTextureCompression;
    bool dyn = actualDevice.supportsDynamicLibraries;
    bool ray = actualDevice.supportsRaytracing;
    bool fnptr = actualDevice.supportsFunctionPointers;
    int rw = actualDevice.readWriteTextureSupport;
    ImGui::Checkbox("BC Textures",&bc);
    ImGui::Checkbox("Dynamic libraries",&dyn);
    ImGui::Checkbox("Raytracing",&ray);
    ImGui::Checkbox("Function Pointers",&fnptr);
    ImGui::DragInt("Texture R/W tier support:",&rw);

    ImGui::Separator();
  }
}

}