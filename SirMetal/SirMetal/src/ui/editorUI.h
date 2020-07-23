#pragma once

#import "imgui.h"

namespace SirMetal {
    class EditorUI {
        struct DockIDs {
            ImGuiID root = 0;
            ImGuiID bottom = 0;
            ImGuiID left = 0;
            ImGuiID right = 0;
            ImGuiID center = 0;


        };

    public:
        bool show(int width, int height);

        inline ImVec2 getViewportSize()const{return viewportPanelSize;};

    private:
        void setupDockSpaceLayout(int width, int height);

    private:
        DockIDs dockIds{};
        ImVec2 viewportPanelSize = {256,256};


    };
}


