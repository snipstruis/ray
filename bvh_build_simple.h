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

    // as we have a single node, it should be a leaf
    assert(bvh->root().isLeaf());

    for (unsigned int i = 0; i < s.primitives.triangles.size(); i++)
        bvh->indicies[i] = i;

    calcAABB(bvh->root().bounds, s.primitives.triangles, 0, bvh->root().count);

    std::cout << "stupid AABB " << bvh->root().bounds << std::endl;
    return bvh;
}

void subdivide(TriangleSet const& triangles, BVH& bvh, BVHNode& node, unsigned int start, 
        unsigned int count, int axis) {

    if(count <= 3) {
        // ok, leafy time.
        node.leftFirst = start;
        // give this node a count, which by definition makes it a leaf
        node.count = count;
        assert(node.isLeaf());

        calcAABB(node.bounds, triangles, node.first(), node.count);
        std::cout << "leaf AABB " << node.bounds << std::endl;
    }
    else {
        // non-leaf node.
        BVHNode left = bvh.allocNextNode();
        BVHNode right = bvh.allocNextNode();
        
        // FIXME: danger, check for odd count
        unsigned int halfCount = count / 2;
        int nextAxis = (axis + 1) % 3;

        // recurse
        subdivide(triangles, bvh, left, start , halfCount, nextAxis);
        subdivide(triangles, bvh, right, start + halfCount, halfCount, nextAxis);

        // now subdivide's done, combine aabb 
        combineAABB(node.bounds, left.bounds, right.bounds);
        std::cout << "comb AABB " << node.bounds << std::endl;
    }
}

// this one is not much better, but does subdivide.
inline BVH* buildSimpleBVH(Scene& s) {
    BVH* bvh = new BVH(s.primitives.triangles.size());

    bvh->root().leftFirst = 0;
    bvh->root().count = 0;

    for (unsigned int i = 0; i < s.primitives.triangles.size(); i++)
        bvh->indicies[i] = i;

    subdivide(s.primitives.triangles, *bvh, bvh->root(), 0, s.primitives.triangles.size(), 0);

    std::cout << "AABB " << bvh->root().bounds << std::endl;

    return bvh;
}

inline BVH* buildBVH(Scene& s) {
//    return buildStupidBVH(s);
    return buildSimpleBVH(s);
}
