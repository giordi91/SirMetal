#include "hierarchyWidget.h"
#include "SirMetalLib/world/hierarchy.h"
#import "imgui.h"
#import "SirMetalLib/engineContext.h"
#import "SirMetalLib/resources/meshes/meshManager.h"
#import "SirMetalLib/core/logging/log.h"


namespace SirMetal {
    namespace Editor {
        static constexpr char *DRAG_KEY = "drag-drop";
        struct ReparentOP {
            uint32_t sourceID;
            uint32_t targetID;
            bool requested = false;
            bool itemClickedThisFrame = false;
        };

        void processChildren(Hierarchy *hierarchy, const DenseTreeNode &node, ReparentOP &op) {

            const std::vector<DenseTreeNode> &nodes = hierarchy->getNodes();
            MeshHandle handle{node.id};
            ImGui::PushID(node.id);
            if (node.index == 0) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            }
            const std::string &meshName = node.index == 0 ? "Root" : SirMetal::CONTEXT->managers.meshManager->getMeshData(handle)->name;


            //if the selection in the hierarchy is the same as the current node
            //we set the node flag to selected
            ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow;
            int selectedIndex = hierarchy->getSelectedId();
            bool isSelected = selectedIndex == node.index;
            node_flags |= isSelected ? ImGuiTreeNodeFlags_Selected : 0;

            //we use the lower version of the TreeNode to pass the flags
            bool expanded = ImGui::TreeNodeEx((void *) (intptr_t) node.index, node_flags, "%s", meshName.c_str());
            //if the node is selected we update the selection for next frame
            bool isClicked = ImGui::IsItemClicked();
            if (isClicked) {
                //if it is the root we simply clear the selection
                if (node.index == 0) {
                    hierarchy->clearSelection();
                } else {
                    hierarchy->select(node.index);
                }
                op.itemClickedThisFrame |= true;
            }


            //dealing with drag and drop
            //if we start the drag we store which object we are moving around
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                //storing the node index so is easy to look up later
                ImGui::SetDragDropPayload(DRAG_KEY, &node.index, sizeof(uint32_t));
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(DRAG_KEY)) {
                    IM_ASSERT(payload->DataSize == sizeof(uint32_t));
                    uint32_t payload_n = *(const uint32_t *) payload->Data;
                    op.requested = true;
                    op.sourceID = payload_n;
                    op.targetID = node.index;
                }
                ImGui::EndDragDropTarget();

            }
            ImGui::PopID();

            //processing children
            if (expanded) {
                for (uint32_t child : node.children) {
                    processChildren(hierarchy, nodes[child], op);
                }
                ImGui::TreePop();
            }


        }


        void Editor::HierarchyWidget::render(Hierarchy *hierarchy, bool *showWindow) {
            //here we check if the mouse was clicked and our window was in focus
            bool windowClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left) & ImGui::IsWindowFocused();

            ReparentOP op{0, 0, 0, 0};
            const std::vector<DenseTreeNode> &nodes = hierarchy->getNodes();


            processChildren(hierarchy, nodes[0], op);
            if (op.requested) {
                std::vector<DenseTreeNode> &nodes = hierarchy->getNodes();
                DenseTreeNode &willBeChild = nodes[op.sourceID];
                DenseTreeNode &willBeParent = nodes[op.targetID];
                hierarchy->parent(willBeParent, willBeChild);
            }
            if (!op.itemClickedThisFrame && windowClicked) {
                hierarchy->clearSelection();
            }
        }
    }
}
