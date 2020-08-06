//
// Created by Marco Giordano on 23/07/2020.
// Copyright (c) 2020 Marco Giordano. All rights reserved.
//

#import <iostream>

#import "engineContext.h"
#import "editorUI.h"
#import "imgui_internal.h"
#import "log.h"
#import "project.h"
#import "../core/flags.h"

namespace SirMetal {
    namespace Editor {
        void EditorUI::show(int width, int height) {
            setupDockSpaceLayout(width, height);

            auto id = ImGui::GetID("Root_Dockspace");

            static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

            ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

            ImGuiViewport *viewport = ImGui::GetMainViewport();

            ImGui::SetNextWindowPos(viewport->GetWorkPos());
            ImGui::SetNextWindowSize(viewport->GetWorkSize());
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

            if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
                window_flags |= ImGuiWindowFlags_NoBackground;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

            ImGui::Begin("Editor", (bool *) 0, window_flags);

            ImGui::PopStyleVar();
            ImGui::PopStyleVar(2);

            static bool showCamera = false;
            //shorcut_menu_bar();
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    ImGui::MenuItem("Open");
                    ImGui::MenuItem("Save");
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Settings")) {

                    if (ImGui::BeginMenu("Docking")) {
                        if (ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0)) dockspace_flags ^= ImGuiDockNodeFlags_NoSplit;
                        if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0)) dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
                        if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0)) dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
                        if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0)) dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
                        if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0)) dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
                        ImGui::EndMenu();
                    }
                    if (ImGui::Button("Camera Settings")) {
                        showCamera = true;
                    }
                    ImGui::EndMenu();
                }

                ImGui::EndMenuBar();
            }

            ImGui::DockSpace(id, ImVec2(0.0f, 0.0f), dockspace_flags
            );


            ImGui::SetNextWindowDockID(dockIds
                    .root, ImGuiCond_Appearing);
            ImGui::Begin("Viewport", (bool *) 0);

            //if our viewport is hovered we set the flag, that will allow
            //our camera controller to behave properly
            bool isWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow);
            setFlagBitfield(CONTEXT->flags.interaction,InteractionFlagsBits::InteractionViewportFocused,isWindowFocused);

            ImVec2 newViewportSize = ImGui::GetContentRegionAvail();
            bool viewportSizeChanged = newViewportSize.x != viewportPanelSize.x || newViewportSize.y != viewportPanelSize.y;
            setFlagBitfield(CONTEXT->flags.viewEvents, ViewEventsFlagsBits::ViewEventsViewportSizeChanged,viewportSizeChanged);
            viewportPanelSize = newViewportSize;
            ImGui::Image(SirMetal::CONTEXT->viewportTexture, viewportPanelSize);
            ImGui::End();

            ImGui::SetNextWindowDockID(dockIds
                    .bottom, ImGuiCond_Appearing);
            ImGui::Begin("Assets", (bool *) 0);
            ImGui::Text("assets go here");

            ImGui::End();

            ImGui::SetNextWindowDockID(dockIds
                    .bottom, ImGuiCond_Appearing);
            ImGui::Begin("log", (bool *) 0);
            const std::string *buff = Log::getBuffer();
            ImGui::Text(buff->c_str());
            ImGui::End();

            ImGui::SetNextWindowDockID(dockIds
                    .left, ImGuiCond_Appearing);
            ImGui::Begin("Hierarchy", (bool *) 0);
            ImGui::Text("see you hierarchy here");

            ImGui::End();

            ImGui::SetNextWindowDockID(dockIds
                    .right, ImGuiCond_Appearing);
            ImGui::Begin("Inspector", (bool *) 0);
            ImGui::Text("inspect values of selected objects here");

            ImGui::End();

            ImGui::End();

            if (showCamera) {
                m_cameraSettings.render(&Editor::PROJECT->getSettings().m_cameraConfig, &showCamera);
            }
            //ImGui::ShowDemoWindow((bool*)0);

        }

        void EditorUI::setupDockSpaceLayout(int width, int height) {
            if (ImGui::DockBuilderGetNode(dockIds.root) == NULL) {
                dockIds.root = ImGui::GetID("Root_Dockspace");
                std::cout << "create layout" << std::endl;

                ImGui::DockBuilderRemoveNode(dockIds.root);                            // Clear out existing layout
                ImGui::DockBuilderAddNode(dockIds.root, ImGuiDockNodeFlags_DockSpace); // Add empty node
                ImGui::DockBuilderSetNodeSize(dockIds.root, ImVec2(width, height));

                dockIds.right = ImGui::DockBuilderSplitNode(dockIds.root, ImGuiDir_Right, 0.2f, NULL, &dockIds.root);
                dockIds.left = ImGui::DockBuilderSplitNode(dockIds.root, ImGuiDir_Left, 0.2f, NULL, &dockIds.root);
                dockIds.bottom = ImGui::DockBuilderSplitNode(dockIds.root, ImGuiDir_Down, 0.3f, NULL, &dockIds.root);

                ImGui::DockBuilderDockWindow("Edit Viewport", dockIds.root);
                ImGui::DockBuilderDockWindow("Play Viewport", dockIds.root);
                ImGui::DockBuilderDockWindow("Log", dockIds.bottom);
                ImGui::DockBuilderDockWindow("Asset Browser", dockIds.bottom);
                ImGui::DockBuilderDockWindow("Scene Hierarchy", dockIds.left);
                ImGui::DockBuilderDockWindow("Inspector", dockIds.right);
                ImGui::DockBuilderFinish(dockIds.root);
            }
        }
    }
}
