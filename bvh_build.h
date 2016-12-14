#pragma once

#include "bvh.h"

template <class Splitter>
void subdivide(
        TriangleSet const& triangles, 
        BVH& bvh, 
        BVHNode& node, 
        unsigned int start, 
        unsigned int count, 
        unsigned int lastAxis) {

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

        // swap elements to be on the correct side of the split point
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
    BVH* bvh = new BVH(s.primitives.triangles.size());

    // setup index map. bvh constructor should have resized indicies
    assert(bvh->indicies.size() >= s.primitives.triangles.size());
    for (unsigned int i = 0; i < s.primitives.triangles.size(); i++)
        bvh->indicies[i] = i;

    // recurse and subdivide
    subdivide<Splitter>(s.primitives.triangles, *bvh, bvh->root(), 0, s.primitives.triangles.size(), 2);
    return bvh;
}

// Build a bad, but valid, BVH. This will have a single root node containing all triangles,
// so traversing it will degrade to a linear search. 
// Useful for testing worst case scenarios, or feeling bad about yourself.
struct StupidSplitter {
    static bool GetSplit(
            unsigned int& axis,             // out: axis on which to split
            float& splitPoint,              // out: point on this axis at which to split
            TriangleSet const& triangles,   // in: master triangle array
            BVH& bvh,                       // in: BVH root 
            BVHNode& node,                  // in: current bvh node (to split)
            std::uint32_t start,            // in: start index in triangle array
            std::uint32_t count,            // in: num triangles (ie start+count <= triangles.size())
            unsigned int lastAxis) {        // in: the axis on which the parent was split

        return false; // stop splitting
    }
};

inline BVH* buildStupidBVH(Scene& s) {
    std::cout << "building stupid BVH" << std::endl;
    BVH* bvh = buildBVH<StupidSplitter>(s);

    // as we have a single node, it should be a leaf
    assert(bvh->nodeCount() == 1);
    assert(bvh->root().isLeaf());
    return bvh;
}

struct MedianSplitter {
    static bool GetSplit(
            unsigned int& axis,             
            float& splitPoint,              
            TriangleSet const& triangles,   
            BVH& bvh,                       
            BVHNode& node,                  
            std::uint32_t start,            
            std::uint32_t count,            
            unsigned int lastAxis) {
        if(count <= 3)
            return false; // don't split

        // blindly move to next axis
        axis = (lastAxis + 1) % 3;

        std::vector<float> v;
        v.reserve(count);

        for (std::uint32_t i = start; i < (start + count) ; i++) {
            unsigned int index = bvh.indicies[i];
            Triangle const& t = triangles[index];
            v.emplace_back(t.getCentroid()[axis]);
        }

        // find median
        std::nth_element(v.begin(), v.begin() + (v.size()/2), v.end());
        splitPoint = v[count/2];

        return true; // do split
    }
};

// this one is not much better, but does subdivide.
inline BVH* buildMedianBVH(Scene& s) {
    std::cout << "building median BVH" << std::endl;
    BVH* bvh = buildBVH<MedianSplitter>(s);
    return bvh;
}

inline BVH* getBVH(Scene& s) {
//    BVH* bvh = buildStupidBVH(s);
    BVH* bvh = buildMedianBVH(s);

    std::cout << "bvh node count " << bvh->nextFree << std::endl;
    std::cout << "triangle count " << s.primitives.triangles.size() << std::endl;
    std::cout << "root AABB " << bvh->root().bounds << std::endl;

    sanityCheckBVH(*bvh, s.primitives.triangles);

    return bvh;
}
