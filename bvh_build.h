#pragma once

#include "bvh.h"
#include "scene.h"

enum BVHMethod {
    BVHMethod_STUPID,
    BVHMethod_MEDIAN,
    __BVHMethod_MAX
};

const char* BVHMethodStr(BVHMethod m) {
    switch(m) {
        case BVHMethod_STUPID: return "STUPID";
        case BVHMethod_MEDIAN: return "MEDIAN";
        case __BVHMethod_MAX: return "shouldnt happen";
    };
}

template <class Splitter>
void subdivide(
        TrianglePosSet const& triangles, 
        BVH& bvh, 
        BVHNode& node, 
        unsigned int start, 
        unsigned int count, 
        unsigned int lastAxis) {
    
    assert(count > 0);
    assert(start + count <= bvh.indicies.size());

    // out params for the splitter. Note they are only defined if shouldSplit == true
    unsigned int splitAxis; 
    float splitValue;

    bool shouldSplit = Splitter::GetSplit(splitAxis, splitValue, triangles, bvh, node, start, count, lastAxis);

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
       
        // we can't split a single triangle - shouldn't have recursed this far.
        // it's arguable we should have never gotten this deep (3's a good limit)
        assert(count > 1);
        assert(splitAxis < 3);

        // alloc child nodes
        node.leftFirst = bvh.nextFree;
        BVHNode& left = bvh.allocNextNode();
        BVHNode& right = bvh.allocNextNode();

        std::cout << "subdiv start " << start << " count " << count << " axis " << splitAxis; 
        std::cout<< " splitVal " << splitValue;
        // partition elements around the splitValue
        int i = start;
        int j = start + count - 1;

        while (i < j) {
            if(triangles[bvh.indicies[i]].getCentroid()[splitAxis] < splitValue) {
                i++;
                continue;
            }
            if(!(triangles[bvh.indicies[j]].getCentroid()[splitAxis] < splitValue)) {
                j--;
                continue;
            }
            std::swap(bvh.indicies[i], bvh.indicies[j]);
            i++;
        }
        // i now points at the first elem of the right group
        unsigned int leftCount = i - start;
        unsigned int rightCount = start + count - i;
        unsigned int rightStart = i;
        
        std::cout << " leftcount " << leftCount << " rightCount " << rightCount << " rightStart " << rightStart;
        std::cout<<std::endl;

        assert(leftCount > 0);
        assert(rightCount > 0);
        assert(leftCount + rightCount == count);
        assert(rightStart > start);
        assert(rightStart + rightCount == start + count);
        
        // recurse
        subdivide<Splitter>(triangles, bvh, left, start, leftCount, splitAxis);
        subdivide<Splitter>(triangles, bvh, right, rightStart, rightCount, splitAxis);

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
    subdivide<Splitter>(s.primitives.pos, *bvh, bvh->root(), 0, s.primitives.triangles.size(), 2);
    return bvh;
}

// Build a bad, but valid, BVH. This will have a single root node containing all triangles,
// so traversing it will degrade to a linear search. 
// Useful for testing worst case scenarios, or feeling bad about yourself.
struct StupidSplitter {
    static bool GetSplit(
            unsigned int& axis,             // out: axis on which to split
            float& splitPoint,              // out: point on this axis at which to split
            TrianglePosSet const& triangles,   // in: master triangle array
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

struct AverageSplitter {
    static bool GetSplit(
            unsigned int& axis,             
            float& splitPoint,              
            TrianglePosSet const& triangles,   
            BVH& bvh,                       
            BVHNode& node,                  
            std::uint32_t start,            
            std::uint32_t count,            
            unsigned int lastAxis) {
        if(count <= 3)
            return false; // don't split

        glm::vec3 total;
        glm::vec3 min(INFINITY, INFINITY, INFINITY);
        glm::vec3 max(-INFINITY, -INFINITY, -INFINITY);

        for (std::uint32_t i = start; i < (start + count) ; i++) {
            unsigned int index = bvh.indicies[i];
            TrianglePosition const& t = triangles[index];
            auto centroid = t.getCentroid();

            total += centroid;
            min = glm::min(min, centroid);
            max = glm::max(max, centroid);
        }

        // find greatest len axis
        // FIXME: there must be a lib funciton to do this
        glm::vec3 lengths = max - min;
        if(lengths[0] > lengths[1])
            axis = 0;
        else
            axis = 1;

        if(lengths[2] > lengths[axis])
            axis = 2;

        std::cout << " min " << min << " max " << max<< std::endl;
        std::cout << " lengths " << lengths << " total " << total << std::endl;
        // and return the average of the greatest axis..
        splitPoint = total[axis] * (1.0f/count);

        return true; // do split
    }
};

// this one is not much better, but does subdivide.
inline BVH* buildMedianBVH(Scene& s) {
    std::cout << "building median BVH" << std::endl;
    BVH* bvh = buildBVH<AverageSplitter>(s);
    return bvh;
}

inline BVH* buildBVH(Scene& s, BVHMethod method) {
    BVH* bvh = nullptr;

    switch(method) {
        case BVHMethod_STUPID: bvh = buildStupidBVH(s); break;
        case BVHMethod_MEDIAN: bvh = buildMedianBVH(s); break;
        case __BVHMethod_MAX: assert(false); break; // shouldn't happen
    };

    std::cout << "bvh node count " << bvh->nextFree << std::endl;
    std::cout << "triangle count " << s.primitives.triangles.size() << std::endl;
    std::cout << "root AABB " << bvh->root().bounds << std::endl;

    sanityCheckBVH(*bvh, s.primitives.pos);

    return bvh;
}
