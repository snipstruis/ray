#pragma once

#include "aabb.h"
#include "primitive.h"
#include "debug_print.h"

#include "glm/vec3.hpp"

#include <iostream>
#include <vector>

// this file contains the core BVH machinery for storage 
// It doesn't contain any BVH building or storage code

// BVHNode
struct BVHNode {
    BVHNode(): leftFirst(0), count(0) {}
    BVHNode(std::uint32_t _leftFirst, std::uint32_t _count): leftFirst(_leftFirst), count(_count) {}

    bool isLeaf() const {
        return count > 0;
    }

    // only valid for non-leaves
    std::uint32_t leftIndex() const {
        assert(!isLeaf());
        return leftFirst;
    }

    // only valid for non-leaves
    std::uint32_t rightIndex() const {
        assert(!isLeaf());
        return leftFirst + 1;
    }

    // only valid for leaves
    std::uint32_t first() const {
        assert(isLeaf());
        return leftFirst;
    }

    AABB bounds;
    std::uint32_t leftFirst;
    std::uint32_t count;
};

// check sizes are as expected - prevent accidental cache performance degredation
static_assert(sizeof(BVHNode) == 32, "BVHNode size");

struct BVH {
    BVH() : nextFree(2) {
        nodes.resize(1); // ensure at least root exists
    } 

    BVH(unsigned int triangleCount) : nextFree(2) {
        nodes.resize(triangleCount* 2); 
        indicies.resize(triangleCount);
    } 

    BVHNode const& getNode(unsigned int index) const {
        assert(index < nodes.size());
        // assert(index < nextFree);
        assert(index != 1); // index 1 is never allocated
        return nodes[index];
    }

    BVHNode& root() {
        assert(nodes.size() > 0);
        return nodes[0];
    }

    BVHNode const& root() const {
        assert(nodes.size() > 0);
        return nodes[0];
    }

    BVHNode& allocNextNode() {
        assert(nextFree < nodes.size());
        return nodes[nextFree++];
    }

    std::uint32_t nodeCount() const {
        assert(nextFree >= 2);
        return nextFree - 1; // includes root node, but skips the empty 1 node
    }

    std::vector<BVHNode> nodes;
    TriangleMapping indicies;
    std::uint32_t nextFree;
};

// recursively check that every node fully contains its child bounds
void sanityCheckAABBRecurse(BVH const& bvh, BVHNode const& node, AABB const& aabb) {
    if(node.isLeaf())
        return;

    auto const& left = bvh.getNode(node.leftIndex());
    auto const& right = bvh.getNode(node.rightIndex());

    assert(containsAABB(aabb, left.bounds));
    assert(containsAABB(aabb, right.bounds));

    // firstly check THIS aabb against every child aabb
    sanityCheckAABBRecurse(bvh, left, aabb);
    sanityCheckAABBRecurse(bvh, right, aabb);

    sanityCheckAABBRecurse(bvh, left, left.bounds);
    sanityCheckAABBRecurse(bvh, right, right.bounds);
}

// walk the whole BVH, explode if the bvh is insane. 
// should compile out on release builds
// this is debug only code, it's certainly not especially efficient
// see also sanityCheck() below for a version that automatically compiles out 
void doSanityCheckBVH(BVH& bvh, TriangleSet const& triangles) {
    // check triangle refs are sane
    for(std::uint32_t i : bvh.indicies) {
        assert(i < triangles.size());
        
        // check uniqueness
        bool found = false;
        for(std::uint32_t j : bvh.indicies) {
            if(found) {
                assert(j != i);
            } 
            else {
                found = (i == j);
            }
        }
    }

    assert(bvh.nodes.size() >= bvh.nodeCount() + 1);

    // check root node
    if (bvh.root().isLeaf()) {
        assert(bvh.nodeCount() == 1);
    }
    else {
        // first non-root node should be at 2
        assert(bvh.root().leftFirst == 2);
        // walk nodes
        for(std::uint32_t i = 2; i < bvh.nodeCount() + 1; i++) {
            auto const& node = bvh.getNode(i);
            std::cout << "i : " << i;

            if(node.isLeaf()) {
                std::cout << " first: " << node.first();
                std::cout << " count: " << node.count;

                assert(node.leftFirst < bvh.indicies.size());
                assert(node.leftFirst + node.count <= bvh.indicies.size());
            } 
            else {
                std::cout << " left: " << node.leftIndex();
                std::cout << " right: " << node.rightIndex();
                assert(node.leftFirst > i);
                assert(node.leftFirst < bvh.nodeCount() + 2);
            }
            std::cout << std::endl;
        }
    }
    std::cout << "sanity check OK" <<std::endl;

    sanityCheckAABBRecurse(bvh, bvh.root(), bvh.root().bounds);
}

void sanityCheckBVH(BVH& bvh, TriangleSet const& triangles) {
#ifndef NDEBUG
    doSanityCheckBVH(bvh, triangles);
#endif
}

