//
// Created by Marco Giordano on 23/07/2020.
// Copyright (c) 2020 Marco Giordano. All rights reserved.
//

#ifndef SIRMETAL_EDITORUI_H
#define SIRMETAL_EDITORUI_H


#import "imgui.h"

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


#endif //SIRMETAL_EDITORUI_H
