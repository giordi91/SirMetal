#pragma once

#include <vector>
#include <cstdint>
#import <queue>
#import <unordered_set>
#import <stack>

namespace SirMetal {
    struct DenseTreeNode {
        static constexpr uint32_t PARENT_NULL = std::numeric_limits<uint32_t>::max();
        uint32_t id;
        uint32_t index;
        uint32_t parentIndex = PARENT_NULL;
        //Change this for random allocator handle, or anything else.
        //anything is better than a vector here
        std::vector<uint32_t> children;
    };

    class DenseTree {
    public:
        void initialize(uint32_t initialSize) {
            m_nodes.reserve(initialSize);
        }

        DenseTreeNode &createNode(DenseTreeNode &parent, uint32_t id) {
            DenseTreeNode &node = createNode(id);
            parentNode(parent, node);
            return node;
        };

        DenseTreeNode &createRoot(uint32_t id) {
            DenseTreeNode &root = createNode(id);
            m_root = &root;
            return root;
        };


        void parentNode(DenseTreeNode &parent, DenseTreeNode &children) {
            assert(parent.index < m_nodes.size());
            assert(children.index < m_nodes.size());

            parent.children.push_back(children.index);

            if (children.parentIndex != DenseTreeNode::PARENT_NULL) {
                assert(children.parentIndex < m_nodes.size());
                DenseTreeNode &childrenParent = m_nodes[children.parentIndex];
                unparentNode(childrenParent, children);
            }

            children.parentIndex = parent.index;
            m_isSorted = false;
        }

        bool isDirectParent(const DenseTreeNode &parent, const DenseTreeNode &child) const {
            return parent.index == child.parentIndex;
        }

        bool isDirectChild(const DenseTreeNode &parent, const DenseTreeNode &child) const {
            size_t count = parent.children.size();
            bool result = false;
            for (size_t i = 0; i < count; ++i) {
                result |= parent.children[i] == child.index;
            }
            return result;
        }

        std::vector<DenseTreeNode> &getNodes() {
            return m_nodes;
        };

        DenseTreeNode *getRoot() {
            return m_root;
        }

        //this can be done externally, for now I will be doing it as member class
        //to make it simpler and more efficient
        void depthFirstSort() {
            //we simply rewrite the indices then ask the std::sort to sort based
            //on that
            if (m_isSorted) {return;}

            m_sortIndices.push(m_root->index);
            m_visitedNodes.clear();
            int counter = 0;
            while (!m_sortIndices.empty()) {
                const uint32_t idx = m_sortIndices.top();
                DenseTreeNode &node = m_nodes[idx];
                node.index = counter++;
                m_sortIndices.pop();
                m_visitedNodes.insert(idx);
                for (uint32_t child : node.children) {
                    if (m_visitedNodes.find(child) == m_visitedNodes.end()) {
                        m_sortIndices.push(child);
                    }
                }
            }

            std::sort(m_nodes.begin(), m_nodes.end(),
                    [](const DenseTreeNode &lhs, const DenseTreeNode &rhs) {
                        return lhs.index < rhs.index;
                    });
            m_isSorted = true;

        }

        bool isSorted() const {
            return m_isSorted;
        }

    private:
        DenseTreeNode &createNode(uint32_t id) {
            m_nodes.emplace_back(DenseTreeNode{id, static_cast<uint32_t>(m_nodes.size())});
            DenseTreeNode &node = m_nodes[m_nodes.size() - 1];
            m_isSorted = false;
            return node;

        };

        void unparentNode(DenseTreeNode &parent, DenseTreeNode &child) {
            int index = -1;
            size_t count = parent.children.size();
            for(size_t i=0; i < count; ++i)
            {
                if(parent.children[i] == child.index)
                {
                    index = i;
                }
            }
            assert(index != -1);
            if(index != -1) {
                //patching element from last
                parent.children[index] = parent.children[count-1];
                parent.children.pop_back();
                child.parentIndex = DenseTreeNode::PARENT_NULL;
                m_isSorted = false;
            }

        }

    private:
        bool m_isSorted = true;
        std::vector<DenseTreeNode> m_nodes;
        DenseTreeNode *m_root = nullptr;
        std::stack<uint32_t> m_sortIndices;
        std::unordered_set<uint32_t> m_visitedNodes;
    };


}