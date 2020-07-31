#pragma once

#import "imgui.h"
#import "cameraSettingsWidget.h"

namespace SirMetal {
    namespace Editor {
        class EditorUI {
            struct DockIDs {
                ImGuiID root = 0;
                ImGuiID bottom = 0;
                ImGuiID left = 0;
                ImGuiID right = 0;
                ImGuiID center = 0;
            };

        public:
            //returns whether or not the viewport size changed
            bool show(int width, int height);

            inline ImVec2 getViewportSize() const {
                return viewportPanelSize;
            };

        private:
            void setupDockSpaceLayout(int width, int height);

        private:
            DockIDs dockIds{};
            ImVec2 viewportPanelSize = {256, 256};
            Editor::CameraSettingsWidget m_cameraSettings;
        };
    }
}


