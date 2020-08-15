
#include "hierarchy.h"

namespace SirMetal {
    DenseTreeNode& Hierarchy::addToRoot(SirMetal::MeshHandle object) {
        DenseTreeNode *root = m_tree.getRoot();
        uint64_t raw = object.handle;
        return m_tree.createNode(*root, object.handle, reinterpret_cast<void *>(raw));
    }

    DenseTreeNode &Hierarchy::addToNode(DenseTreeNode& node, MeshHandle handle) {
        uint64_t raw = handle.handle;
        return m_tree.createNode(node, handle.handle, reinterpret_cast<void *>(raw));
    }

    void Hierarchy::parent(DenseTreeNode &parent, DenseTreeNode &child) {
        m_tree.parentNode(parent,child);
    }

}