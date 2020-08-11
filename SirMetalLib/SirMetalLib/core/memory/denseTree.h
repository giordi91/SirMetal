#pragma once

#include <vector>
#include <cstdint>

namespace SirMetal {
    struct DenseTreeNode {
        uint32_t id;
        uint32_t index;
        //Change this for random allocator handle
        std::vector<uint32_t> children;
    };

    class DenseTree {
        void initialize(uint32_t capacity) {
            m_nodes.reserve(capacity);
        }

        DenseTreeNode createNode() {
            if (m_allocatedNodes >= m_nodes.size()) {grow();}
            m_nodes[m_allocatedNodes] = {};
            return m_nodes[m_allocatedNodes];
        };

        void parentNode(uint32_t parentId, uint32_t childrenId) {
            assert(parentId < m_nodes.size());
            assert(childrenId < m_nodes.size());

            DenseTreeNode& parent = m_nodes[parentId];
            parent.children.push_back(childrenId);
        }

    private:
        void grow() {
            size_t currSize = m_nodes.size() == 0 ? 1 : m_nodes.size();
            m_nodes.resize(currSize * 2);
        };

    private:
        uint32_t m_allocatedNodes = 0;
        std::vector<DenseTreeNode> m_nodes;
    };

}