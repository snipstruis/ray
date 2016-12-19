#pragma once

#include "bvh.h"
#include "primitive.h"

#include <iostream>

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

