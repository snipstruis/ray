#pragma once

#include "bvh.h"

// This file contains simple and reference implentations for bvh building
// They probably aren't optimal

// Build a bad, but valid, BVH. This will have a single root node containing all triangles,
// so traversing it will degrade to a linear search. 
// Useful for testing worst case scenarios, or feeling bad about yourself.
inline BVH* buildStupidBVH(Scene& s) {
    BVH* bvh = new BVH;

    bvh->root().leftFirst = 0;
    bvh->root().count = s.primitives.triangles.size();

    bvh->indicies.resize(s.primitives.triangles.size());

    for (unsigned int i = 0; i < s.primitives.triangles.size(); i++)
        bvh->indicies[i] = i;

    calcAABB(bvh->root().bounds, s.primitives.triangles, 0, s.primitives.triangles.size());

    std::cout << "AABB " << bvh->root().bounds << std::endl;
    return bvh;
}

inline BVH* buildBVH(Scene& s) {
    return buildStupidBVH(s);
}
