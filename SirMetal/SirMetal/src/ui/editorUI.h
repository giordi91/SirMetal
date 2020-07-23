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
        void show(int width, int height);

    private:
        void setupDockSpaceLayout(int width, int height);

    private:
        DockIDs dockIds{};
    };
}


