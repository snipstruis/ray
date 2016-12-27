#pragma once

#include "bvh.h"
#include "bvh_build_common.h"

// Build a bad, but valid, BVH. This will have a single root node containing all triangles,
// so traversing it will (almost) degrade to a linear search. It still does have an AABB, 
// so rays that miss it completely will still be accelerated.
// Useful for testing worst case scenarios, or feeling bad about yourself.
struct StupidSplitter {
    static bool TrySplit(
            BVH& bvh,                           // in: bvh root
            TrianglePosSet const& triangles,    // in: master triangle array
            TriangleMapping const& indicies,    // in: set of triangle indicies to split 
            AABB const& bounds,                 // in: bounds of this set of triangles
            TriangleMapping& leftIndicies,      // out: resultant left set
            TriangleMapping& rightIndicies) {   // out: resultant right set

        return false; // stop splitting
    }
};

inline BVH* buildStupidBVH(Scene& s) {
    std::cout << "building stupid BVH" << std::endl;
    BVH* bvh = buildBVH<StupidSplitter>(s);

    // For the stupid splitter, we should have a single node, and it should be a leaf
    assert(bvh->nodeCount() == 1);
    assert(bvh->root().isLeaf());
    return bvh;
}

