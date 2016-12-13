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

    calcAABBIndirect(bvh->root().bounds, s.primitives.triangles, bvh->indicies, 0, bvh->root().count);

    std::cout << "stupid AABB " << bvh->root().bounds << std::endl;
    return bvh;
}

void subdivide(TriangleSet const& triangles, BVH& bvh, BVHNode& node, std::uint32_t start, std::uint32_t count, int axis) {

    if(count <= 150) {
        std::cout << "subdivide leaf start " << start << " count " << count << " axis " << axis <<std::endl;;
        // ok, leafy time.
        node.leftFirst = start;
        // give this node a count, which by definition makes it a leaf
        node.count = count;
        assert(node.isLeaf());
        assert(node.leftFirst == node.first());

        calcAABBIndirect(node.bounds, triangles, bvh.indicies, node.first(), node.count);
        std::cout << "leaf AABB " << node.bounds << " count " << node.count << std::endl;
    }
    else {
        std::cout << "subdivide mid start " << start << " count " << count << " axis " << axis <<std::endl;;
        float sum = 0.0f;
        // non-leaf node.
        for (std::uint32_t i = start; i < (start + count) ; i++) {
            assert(i < bvh.indicies.size());
            unsigned int index = bvh.indicies[i];

            assert(index < triangles.size());
            Triangle const& t = triangles[index];

            // do this with centoids?
            sum += t.v[0][axis] + t.v[1][axis] + t.v[2][axis];
        }

        // get average. div by 3*triangles (ie once for each vertex)
        float average = sum / (count * 3.0f);

        std::cout << "av " << average << " axis " << axis << " count " << count << std::endl;

        // FIXME: danger, check for odd count

        int i = start;
        int j = start + count - 1;

        while (i < j) {
            assert(bvh.indicies[i] < triangles.size());
            assert(bvh.indicies[j] < triangles.size());

            if(triangles[bvh.indicies[i]].getCentroid()[axis] < average) {
                i++;
                continue;
            }
            if(triangles[bvh.indicies[j]].getCentroid()[axis] > average) {
                j--;
                continue;
            }

#if 0
            std::cout << "swap " << i << " " << j << std::endl;
            std::cout << "ii " << triangles[bvh.indicies[i]].getCentroid()[axis] << " ";
            std::cout << "jj " << triangles[bvh.indicies[j]].getCentroid()[axis] << " ";
            std::cout << std::endl;
#endif

            int temp = bvh.indicies[j];
            bvh.indicies[j] = bvh.indicies[i];
            bvh.indicies[i] = temp;

            i++;
            j--;
        }

        // alloc child nodes
        node.leftFirst = bvh.nextFree;
        BVHNode& left = bvh.allocNextNode();
        BVHNode& right = bvh.allocNextNode();
        
        std::uint32_t halfCount = count / 2;
        int nextAxis = (axis + 1) % 3;

        // recurse
        subdivide(triangles, bvh, left, start, halfCount, nextAxis);
        subdivide(triangles, bvh, right, (start + halfCount), halfCount, nextAxis);

        // now subdivide's done, combine aabb 
        combineAABB(node.bounds, left.bounds, right.bounds);
    }
}

// this one is not much better, but does subdivide.
inline BVH* buildSimpleBVH(Scene& s) {
    BVH* bvh = new BVH(s.primitives.triangles.size());

    bvh->root().leftFirst = 2;
    bvh->root().count = 0;

    for (unsigned int i = 0; i < s.primitives.triangles.size(); i++)
        bvh->indicies[i] = i;

    subdivide(s.primitives.triangles, *bvh, bvh->root(), 0, s.primitives.triangles.size(), 0);

    std::cout << "node count " << bvh->nextFree << std::endl;
    std::cout << "triangle count " << s.primitives.triangles.size() << std::endl;
    std::cout << "simple AABB " << bvh->root().bounds << std::endl;
    std::cout << "  l=" << bvh->root().leftIndex()<< " r=" << bvh->root().rightIndex()<< std::endl;
    std::cout << " left AABB " << bvh->getNode( bvh->root().leftIndex()).bounds<< std::endl;
    std::cout << " right AABB " << bvh->getNode( bvh->root().rightIndex()).bounds<< std::endl;

    assert(bvh->root().leftFirst ==2);
    assert(bvh->root().count == 0);

    return bvh;
}

inline BVH* buildBVH(Scene& s) {
    BVH* b = buildSimpleBVH(s);
    sanityCheckBVH(*b, s.primitives.triangles);
    return b;
}
