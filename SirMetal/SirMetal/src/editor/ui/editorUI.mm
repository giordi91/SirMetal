//
// Created by Marco Giordano on 23/07/2020.
// Copyright (c) 2020 Marco Giordano. All rights reserved.
//

#import <iostream>
#import <simd/simd.h>

#import "SirMetalLib/engineContext.h"
#import "editorUI.h"
#import "imgui_internal.h"
#import "SirMetalLib/core/logging/log.h"
#import "project.h"
#import "SirMetalLib/core/flags.h"
#import "../../vendors/imguizmo/ImGuizmo.h"
#import "SirMetalLib/MBEMathUtilities.h"
#import "SirMetalLib/resources/meshes/meshManager.h"


namespace SirMetal {
    namespace Editor {

        void EditTransform(const float *cameraView, float *cameraProjection, float *matrix, bool editTransformDecomposition,
                ImVec2 screenCorner, ImVec2 screenWidth) {
            static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
            static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::LOCAL);
            static bool useSnap = false;
            static float snap[3] = {1.f, 1.f, 1.f};
            static float bounds[] = {-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f};
            static float boundsSnap[] = {0.1f, 0.1f, 0.1f};
            static bool boundSizing = false;
            static bool boundSizingSnap = false;

            // 18-19-20 are respectively 1-2-3 number on keyboard
            if (ImGui::IsKeyPressed(49)) {
                mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
            }
            if (ImGui::IsKeyPressed(50)) {
                mCurrentGizmoOperation = ImGuizmo::ROTATE;
            }
            if (ImGui::IsKeyPressed(51)) // r Key
                mCurrentGizmoOperation = ImGuizmo::SCALE;
            /*
            if (editTransformDecomposition) {
                if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
                    mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
                ImGui::SameLine();
                if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
                    mCurrentGizmoOperation = ImGuizmo::ROTATE;
                ImGui::SameLine();
                if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
                    mCurrentGizmoOperation = ImGuizmo::SCALE;
                float matrixTranslation[3], matrixRotation[3], matrixScale[3];
                ImGuizmo::DecomposeMatrixToComponents(matrix, matrixTranslation, matrixRotation, matrixScale);
                ImGui::InputFloat3("Tr", matrixTranslation, 3);
                ImGui::InputFloat3("Rt", matrixRotation, 3);
                ImGui::InputFloat3("Sc", matrixScale, 3);
                ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, matrix);

                if (mCurrentGizmoOperation != ImGuizmo::SCALE) {
                    if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
                        mCurrentGizmoMode = ImGuizmo::LOCAL;
                    ImGui::SameLine();
                    if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
                        mCurrentGizmoMode = ImGuizmo::WORLD;
                }
                if (ImGui::IsKeyPressed(83))
                    useSnap = !useSnap;
                ImGui::Checkbox("", &useSnap);
                ImGui::SameLine();

                switch (mCurrentGizmoOperation) {
                    case ImGuizmo::TRANSLATE:
                        ImGui::InputFloat3("Snap", &snap[0]);
                        break;
                    case ImGuizmo::ROTATE:
                        ImGui::InputFloat("Angle Snap", &snap[0]);
                        break;
                    case ImGuizmo::SCALE:
                        ImGui::InputFloat("Scale Snap", &snap[0]);
                        break;
                }
                ImGui::Checkbox("Bound Sizing", &boundSizing);
                if (boundSizing) {
                    ImGui::PushID(3);
                    ImGui::Checkbox("", &boundSizingSnap);
                    ImGui::SameLine();
                    ImGui::InputFloat3("Snap", boundsSnap);
                    ImGui::PopID();
                }
            }
            */
            ImGuizmo::SetRect(screenCorner.x, screenCorner.y,
                    screenWidth.x, screenWidth.y);
            ImGuizmo::Manipulate(cameraView, cameraProjection, mCurrentGizmoOperation, mCurrentGizmoMode, matrix, NULL, useSnap ? &snap[0] : NULL, boundSizing ? bounds : NULL, boundSizingSnap ? boundsSnap : NULL);
        }


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
            setFlagBitfield(CONTEXT->flags.interaction, InteractionFlagsBits::InteractionViewportFocused, isWindowFocused);

            ImVec2 newViewportSize = ImGui::GetContentRegionAvail();
            bool viewportSizeChanged = (newViewportSize.x != viewportPanelSize.x) | (newViewportSize.y != viewportPanelSize.y);
            setFlagBitfield(CONTEXT->flags.viewEvents, ViewEventsFlagsBits::ViewEventsViewportSizeChanged, viewportSizeChanged);
            viewportPanelSize = newViewportSize;
            //storing the start of the cursor in the window before we add
            //the image, so we can use it to overlay imguizmo
            float x = ImGui::GetCursorScreenPos().x;
            float y = ImGui::GetCursorScreenPos().y;

            ImGui::Image(SirMetal::CONTEXT->viewportTexture, viewportPanelSize);

            bool somethingSelected = CONTEXT->world.hierarchy.getSelectedId() != -1;
            bool isManipulator = somethingSelected;
            if (somethingSelected) {
                Camera &camera = CONTEXT->camera;
                float *view = (float *) &camera.viewInverse;
                float *proj = (float *) &camera.projection;
                //TODO fix temporary access directly to the model matrix, will need to change
                //TODO fix access to mesh hardcoded name, should come from seleection
                auto handle = SirMetal::CONTEXT->managers.meshManager->getHandleFromName("lucy");
                auto *data = SirMetal::CONTEXT->managers.meshManager->getMeshData(handle);
                const matrix_float4x4 *modelMatrix = &data->modelMatrix;
                float *mat = (float *) modelMatrix;

                ImGuizmo::SetDrawlist();
                EditTransform(view, proj, mat, true,
                        {x, y},
                        ImVec2{newViewportSize.x, newViewportSize.y});

                isManipulator &= ImGuizmo::IsUsing();
            }
            setFlagBitfield(CONTEXT->flags.interaction, InteractionFlagsBits::InteractionViewportGuizmo, isManipulator);

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
            ImGui::Text("%s", buff->c_str());
            ImGui::End();

            ImGui::SetNextWindowDockID(dockIds
                    .left, ImGuiCond_Appearing);
            if (ImGui::Begin("Hierarchy", &m_showHierarchy)) {
                if (ImGui::Button("Clear Selection")) {
                    CONTEXT->world.hierarchy.clearSelection();
                }
                m_hierarchy.render(&CONTEXT->world.hierarchy, &m_showHierarchy);

                ImGui::End();
            };

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


            //ImGuiIO &io = ImGui::GetIO();
            //ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
            //ImGuizmo::Manipulate(cameraView, cameraProjection, mCurrentGizmoOperation, mCurrentGizmoMode, matrix, NULL, useSnap ? &snap[0] : NULL, boundSizing ? bounds : NULL, boundSizingSnap ? boundsSnap : NULL);

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

        void EditorUI::show2(uint32_t i, uint32_t i1) {
            Camera &camera = CONTEXT->camera;
            float *view = (float *) &camera.viewInverse;
            float *proj = (float *) &camera.projection;
            auto matrix = matrix_float4x4_translation(vector_float3{0, 0, 0});
            float *mat = (float *) &matrix;

            ImGuiIO &io = ImGui::GetIO();
            EditTransform(view, proj, mat, true, ImVec2{0, 0},
                    ImVec2{io.DisplaySize.x, io.DisplaySize.y});

        }
    }
}
