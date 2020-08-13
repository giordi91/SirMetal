
#include "hierarchy.h"

namespace SirMetal {
    void Hierarchy::addToRoot(SirMetal::MeshHandle object) {
        DenseTreeNode *root = m_tree.getRoot();
        uint64_t raw = object.handle;
        m_tree.createNode(*root, object.handle, reinterpret_cast<void *>(raw));
    }

}