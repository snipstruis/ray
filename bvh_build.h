#pragma once

#include "bvh.h"

#if 0
void subdivide(TriangleSet const& triangles, BVH& bvh, BVHNode& node, std::uint32_t start, std::uint32_t count, int axis) {

    if(count <= 3) {
        // std::cout << "subdivide leaf start " << start << " count " << count << " axis " << axis <<std::endl;;
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
//        std::cout << "subdivide mid start " << start << " count " << count << " axis " << axis <<std::endl;;
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

//        std::cout << "av " << average << " axis " << axis << " count " << count << std::endl;

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

        // beware odd numbers..!
        std::uint32_t secondRange = halfCount + (count % 2);
        
        int nextAxis = (axis + 1) % 3;
        // recurse
        subdivide(triangles, bvh, left, start, halfCount, nextAxis);
        subdivide(triangles, bvh, right, (start + halfCount), secondRange, nextAxis);

        // now subdivide's done, combine aabb 
        combineAABB(node.bounds, left.bounds, right.bounds);
    }
}
#endif

template <class Splitter>
void subdivide(TriangleSet const& triangles, BVH& bvh, BVHNode& node, unsigned int start, unsigned int count, unsigned int lastAxis) {

    // out params for the splitter. Note they are only defined if shouldSplit == true
    unsigned int splitAxis; 
    float splitPoint;

    bool shouldSplit = Splitter::GetSplit(splitAxis, splitPoint, triangles, bvh, node, start, count, lastAxis);

    // are we creating a leaf?
    if(!shouldSplit) {
        // ok, leafy time.
        //std::cout << "subdivide leaf start " << start << " count " << count << " axis " << axis <<std::endl;;
        node.leftFirst = start;

        // give this node a count, which by definition makes it a leaf
        node.count = count;
        assert(node.isLeaf());
        assert(node.leftFirst == node.first());

        calcAABBIndirect(node.bounds, triangles, bvh.indicies, node.first(), node.count);
        std::cout << "leaf AABB " << node.bounds << " count " << node.count << std::endl;
    } else {
        // not creating a leaf, we're splitting - create a subdivision
        assert(splitAxis < 3);
        // we can't split a single triangle - shouldn't have recursed this far.
        // it's arguable we should have never gotten this deep (3's a good limit)
        assert(count > 1);

        int i = start;
        int j = start + count - 1;

        while (i < j) {
            assert(bvh.indicies[i] < triangles.size());
            assert(bvh.indicies[j] < triangles.size());

            if(triangles[bvh.indicies[i]].getCentroid()[splitAxis] < splitPoint) {
                i++;
                continue;
            }
            if(triangles[bvh.indicies[j]].getCentroid()[splitAxis] > splitPoint) {
                j--;
                continue;
            }

            std::swap(bvh.indicies[i], bvh.indicies[j]);

            i++;
            j--;
        }

        // alloc child nodes
        node.leftFirst = bvh.nextFree;
        BVHNode& left = bvh.allocNextNode();
        BVHNode& right = bvh.allocNextNode();
        
        std::uint32_t halfCount = count / 2;

        // beware odd numbers..!
        std::uint32_t secondRange = halfCount + (count % 2);
        
        // recurse
        subdivide<Splitter>(triangles, bvh, left, start, halfCount, splitAxis);
        subdivide<Splitter>(triangles, bvh, right, (start + halfCount), secondRange, splitAxis);

        // now subdivide's done, combine aabb 
        combineAABB(node.bounds, left.bounds, right.bounds);
    }
}

template<class Splitter>
inline BVH* buildBVH(Scene& s) {
    BVH* bvh = new BVH;

    // setup index map
    for (unsigned int i = 0; i < s.primitives.triangles.size(); i++)
        bvh->indicies[i] = i;

    // setup root node
//    bvh->root().leftFirst = 0;
//    bvh->root().count = s.primitives.triangles.size();
//    bvh->indicies.resize(s.primitives.triangles.size());



//    calcAABBIndirect(bvh->root().bounds, s.primitives.triangles, bvh->indicies, 0, bvh->root().count);

 //   std::cout << "stupid AABB " << bvh->root().bounds << std::endl;
    return bvh;
}

// Build a bad, but valid, BVH. This will have a single root node containing all triangles,
// so traversing it will degrade to a linear search. 
// Useful for testing worst case scenarios, or feeling bad about yourself.
struct StupidSplitter {
    bool GetSplit(
            int& axis,                      // out: axis on which to split
            float& splitPoint,              // out: point on this axis at which to split
            TriangleSet const& triangles,   // in: master triangle array
            BVH& bvh,                       // in: BVH root 
            BVHNode& node,                  // in: current bvh node (to split)
            std::uint32_t start,            // in: start index in triangle array
            std::uint32_t count,            // in: num triangles (ie start+count <= triangles.size())
            int lastAxis) {                 // in: the axis on which the parent was split

        return true; // stop splitting
    }
};

inline BVH* buildStupidBVH(Scene& s) {
    BVH* bvh = buildBVH<StupidSplitter>(s);
    // as we have a single node, it should be a leaf
    assert(bvh->nodeCount() == 1);
    assert(bvh->root().isLeaf());
    return bvh;
}

#if 0
// this one is not much better, but does subdivide.
inline BVH* buildSimpleBVH(Scene& s) {
    BVH* bvh = new BVH(s.primitives.triangles.size());

    bvh->root().leftFirst = 2;
    bvh->root().count = 0;

    for (unsigned int i = 0; i < s.primitives.triangles.size(); i++)
        bvh->indicies[i] = i;

    subdivide(s.primitives.triangles, *bvh, bvh->root(), 0, s.primitives.triangles.size(), 0);

    std::cout << "bvh node count " << bvh->nextFree << std::endl;
    std::cout << "triangle count " << s.primitives.triangles.size() << std::endl;
    std::cout << "simple root AABB " << bvh->root().bounds << std::endl;

    assert(bvh->root().leftFirst ==2);
    assert(bvh->root().count == 0);

    return bvh;
}

#endif

inline BVH* getBVH(Scene& s) {
    BVH* b = buildStupidBVH(s);
    sanityCheckBVH(*b, s.primitives.triangles);
    return b;
}
#if 0
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
#endif