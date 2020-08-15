#pragma once

#include "SirMetalLib/core/memory/denseTree.h"
#include "SirMetalLib/resources/handle.h"

namespace SirMetal {
    class Hierarchy {
    public:
        Hierarchy() {
            m_tree.initialize();
            m_tree.createRoot(ROOT_ID, nullptr);
        }
        DenseTreeNode& addToRoot(MeshHandle object);
        std::vector<DenseTreeNode>& getNodes(){return m_tree.getNodes();}

        DenseTreeNode &addToNode(DenseTreeNode& node, MeshHandle handle);
        void parent(DenseTreeNode& parent,DenseTreeNode& child);

    private:
        static constexpr uint32_t ROOT_ID =0;
        DenseTree m_tree;
    };

}