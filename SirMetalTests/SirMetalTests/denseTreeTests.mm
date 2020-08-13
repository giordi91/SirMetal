#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include "SirMetalLib/core/memory/denseTree.h"


TEST_CASE( "dense tree create node", "[memory]" ) {
    SirMetal::DenseTree tree;
    tree.initialize(16);
    SirMetal::DenseTreeNode& node = tree.createRoot(0);

    REQUIRE( node.id == 0);
    REQUIRE( node.index == 0);

    SirMetal::DenseTreeNode& node2 = tree.createNode(node,2);

    REQUIRE( node2.id == 2);
    REQUIRE( node2.index == 1);
}

TEST_CASE( "dense tree basic node parenting", "[memory]" ) {
    SirMetal::DenseTree tree;
    tree.initialize(16);
    SirMetal::DenseTreeNode& node1 = tree.createRoot(1);
    SirMetal::DenseTreeNode& node2 = tree.createNode(node1,2);

    REQUIRE(node2.parentIndex == node1.index);
    REQUIRE(node1.children.size() == 1);
    REQUIRE(node1.children[0] == node2.index);
    REQUIRE(tree.isDirectParent(node1,node2));
    REQUIRE(tree.isDirectChild(node1,node2));
}

TEST_CASE( "dense tree DFS sorting", "[memory]" ) {

    //we will make the same tree as
    //https://en.wikipedia.org/wiki/Depth-first_search#/media/File:Depth-First-Search.gif
    SirMetal::DenseTree tree;
    tree.initialize(16);
    SirMetal::DenseTreeNode& node1 = tree.createRoot(1);
    SirMetal::DenseTreeNode& node2 = tree.createNode(node1,2);
    SirMetal::DenseTreeNode& node5 = tree.createNode(node1,5);
    SirMetal::DenseTreeNode& node9 = tree.createNode(node1,9);

    SirMetal::DenseTreeNode& node3 = tree.createNode(node2,3);
    SirMetal::DenseTreeNode& node6 = tree.createNode(node5,6);
    SirMetal::DenseTreeNode& node8 = tree.createNode(node5,8);
    SirMetal::DenseTreeNode& node10 = tree.createNode(node9,10);

    tree.createNode(node3,4);
    tree.createNode(node6,7);

    std::vector<uint32_t> initialIds = {1,2,5,9,3,6,8,10,4,7};
    auto nodes= tree.getNodes();
    size_t count = nodes.size();
    REQUIRE(count == initialIds.size());
    for(size_t i =0; i <count;++i)
    {
        REQUIRE(nodes[i].id == initialIds[i]);
    }

    REQUIRE(!tree.isSorted());
    tree.depthFirstSort();
    REQUIRE(tree.isSorted() );

    nodes= tree.getNodes();
    std::vector<uint32_t> expectedIds= {1,9,10,5,8,6,7,2,3,4};
    count = nodes.size();
    REQUIRE(count == expectedIds.size());
    for(size_t i =0; i <count;++i)
    {
        REQUIRE(nodes[i].id == expectedIds[i]);
    }
}

TEST_CASE( "dense tree basic node parenting with prvious parent", "[memory]" ) {
    SirMetal::DenseTree tree;
    tree.initialize(16);
    SirMetal::DenseTreeNode &node1 = tree.createRoot(1);
    SirMetal::DenseTreeNode &node2 = tree.createNode(node1, 2);
    SirMetal::DenseTreeNode &node3 = tree.createNode(node1, 2);

    tree.parentNode(node2,node3);

    REQUIRE(tree.isDirectChild(node1,node2));
    REQUIRE(!tree.isDirectChild(node1,node3));
    REQUIRE(!tree.isDirectParent(node1,node3));
    REQUIRE(tree.isDirectChild(node2,node3));
    REQUIRE(tree.isDirectParent(node2,node3));
}


