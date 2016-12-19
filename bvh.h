#pragma once

#include "aabb.h"
#include "primitive.h"

#include <vector>

// this file contains the core BVH machinery for storage 
// It doesn't contain any BVH building or traversal code

// BVHNode
struct BVHNode {
    BVHNode(): leftFirst(0), count(0) {}
    BVHNode(unsigned int _leftFirst, unsigned int _count): leftFirst(_leftFirst), count(_count) {}

    bool isLeaf() const {
        return count > 0;
    }

    // only valid for non-leaves
    unsigned int leftIndex() const {
        assert(!isLeaf());
        return leftFirst;
    }

    // only valid for non-leaves
    unsigned int rightIndex() const {
        assert(!isLeaf());
        return leftFirst + 1;
    }

    // only valid for leaves
    unsigned int first() const {
        assert(isLeaf());
        return leftFirst;
    }

    AABB bounds;
    unsigned int leftFirst;
    unsigned int count;
};

// check sizes are as expected - prevent accidental cache performance degredation
static_assert(sizeof(BVHNode) == 32, "BVHNode size");

struct BVH {
    BVH() : nextFree(2) {
        nodes.resize(1); // ensure at least root exists
    } 

    BVH(unsigned int triangleCount) : nextFree(2) {
        // FIXME:should we be smarter here?
        nodes.resize(triangleCount* 3); 
        //indicies.resize(triangleCount);
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

    unsigned int nodeCount() const {
        assert(nextFree >= 2);
        return nextFree - 1; // includes root node, but skips the empty 1 node
    }

    std::vector<BVHNode> nodes;
    TriangleMapping indicies;
    unsigned int nextFree;
};

