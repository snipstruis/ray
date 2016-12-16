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

// recursively check that every node fully contains its child bounds
void sanityCheckAABBRecurse(BVH const& bvh, BVHNode const& node, TrianglePosSet const& triangles) {
    if(node.isLeaf()) {
        // ensure all triangles are inside aabb
        for(unsigned int i = node.first(); i < (node.first() + node.count); i++) {
            TrianglePosition const& t = triangles[bvh.indicies[i]];
            assert(containsTriangle(node.bounds, t));
        }
    }
    else {
        auto const& left = bvh.getNode(node.leftIndex());
        auto const& right = bvh.getNode(node.rightIndex());

        // check both immediate child bounds fit in our bounds
        assert(containsAABB(node.bounds, left.bounds));
        assert(containsAABB(node.bounds, right.bounds));

        sanityCheckAABBRecurse(bvh, left, triangles);
        sanityCheckAABBRecurse(bvh, right, triangles);
    }
}

// walk the whole BVH, explode if the bvh is insane. 
// should compile out on release builds
// this is debug only code, it's certainly not especially efficient
// see also sanityCheck() below for a version that automatically compiles out 
void doSanityCheckBVH(BVH& bvh, TrianglePosSet const& triangles) {
    // check triangle refs are sane
    for(unsigned int i : bvh.indicies) {
        assert(i < triangles.size());
        
        // check uniqueness
        bool found = false;
        for(unsigned int j : bvh.indicies) {
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

        unsigned int triangleCount = 0;
        // walk nodes
        for(unsigned int i = 2; i < bvh.nodeCount() + 1; i++) {
            auto const& node = bvh.getNode(i);

            if(node.isLeaf()) {
                triangleCount += node.count;
                assert(node.leftFirst < bvh.indicies.size());
                assert(node.leftFirst + node.count <= bvh.indicies.size());
            } 
            else {
                assert(node.leftFirst > i);
                assert(node.leftFirst < bvh.nodeCount() + 2);
            }
        }
        // triangles can be accounted for than once in the bvh. This assert will blow if there's 
        // some missing. but it's no longer actually sufficient. A triangle could be missing all together and
        // another double-counted, and this wouldn't find it. Really need to walk the triangle array, and 
        // search the BVH for every triangle
        assert(triangleCount >= triangles.size());
        std::cout << "bvh triangle count " << triangleCount << " triangles " << triangles.size() << std::endl;
    }

    std::cout << "sanity check recursing " <<std::endl;

    sanityCheckAABBRecurse(bvh, bvh.root(), triangles);

    std::cout << "sanity check OK" <<std::endl;
}

void sanityCheckBVH(BVH& bvh, TrianglePosSet const& triangles) {
#ifndef NDEBUG
    doSanityCheckBVH(bvh, triangles);
#endif
}

