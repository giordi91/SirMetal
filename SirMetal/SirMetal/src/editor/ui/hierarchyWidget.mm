#include "hierarchyWidget.h"
#include "SirMetalLib/world/hierarchy.h"
#import "imgui.h"
#import "SirMetalLib/engineContext.h"
#import "SirMetalLib/resources/meshes/meshManager.h"
#import "SirMetalLib/core/logging/log.h"


namespace SirMetal {
    namespace Editor {
        struct ReparentOP {
            uint32_t sourceID;
            uint32_t targetID;
            bool requested = false;
        };

        void processChildren(Hierarchy *hierarchy, const DenseTreeNode &node, ReparentOP &op) {

            const std::vector<DenseTreeNode> &nodes = hierarchy->getNodes();
            MeshHandle handle{node.id};
            ImGui::PushID(node.id);
            if(node.index ==0)
            {
                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            }
            const std::string &meshName = node.index == 0 ? "Root" : SirMetal::CONTEXT->managers.meshManager->getMeshData(handle)->name;

            bool expanded = ImGui::TreeNode((void *) (intptr_t) node.index, "%s", meshName.c_str());

            // Our buttons are both drag sources and drag targets here!
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                // Set payload to carry the index of our item (could be anything)
                ImGui::SetDragDropPayload("node drag", &node.index, sizeof(uint32_t));
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("node drag")) {
                    IM_ASSERT(payload->DataSize == sizeof(uint32_t));
                    uint32_t payload_n = *(const int *) payload->Data;
                    //MeshHandle handle{payload_n};
                    //const std::string &meshName = SirMetal::CONTEXT->managers.meshManager->getMeshData(handle)->name;
                    op.requested = true;
                    op.sourceID = payload_n;
                    op.targetID = node.index;
                }
                ImGui::EndDragDropTarget();

            }
            ImGui::PopID();
            if (expanded) {
                for (uint32_t child : node.children) {
                    processChildren(hierarchy, nodes[child], op);
                }
                ImGui::TreePop();
            }


        }


        void Editor::HierarchyWidget::render(Hierarchy *hierarchy, bool *showWindow) {
            ReparentOP op{0, 0, 0};
            const std::vector<DenseTreeNode> &nodes = hierarchy->getNodes();


            processChildren(hierarchy,nodes[0],op);
            //if (ImGui::TreeNode("Root")) {
            //    for (uint32_t child : nodes[0].children) {
            //        processChildren(hierarchy, nodes[child], op);
            //    }
            //    ImGui::TreePop();
            //}
            if (op.requested) {
                SIR_CORE_INFO("Reparent requested {} {}", op.sourceID, op.targetID);
                std::vector<DenseTreeNode> &nodes = hierarchy->getNodes();
                DenseTreeNode &willBeChild = nodes[op.sourceID];
                DenseTreeNode &willBeParent = nodes[op.targetID];
                hierarchy->parent(willBeParent, willBeChild);
            }
        }
    }
}
