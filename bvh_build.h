#pragma once

#include "bvh.h"
#include "scene.h"

enum BVHMethod {
    BVHMethod_STUPID,
    BVHMethod_MEDIAN,
    BVHMethod_SAH,
    __BVHMethod_MAX
};

const char* BVHMethodStr(BVHMethod m) {
    switch(m) {
        case BVHMethod_STUPID: return "STUPID";
        case BVHMethod_MEDIAN: return "MEDIAN";
        case BVHMethod_SAH:    return "SAH";
        case __BVHMethod_MAX: return "shouldnt happen";
    };
}

template <class Splitter>
void subdivide(
        TrianglePosSet const& triangles, 
        BVH& bvh, 
        BVHNode& node, 
        TriangleMapping const& fromIndicies,
        unsigned int lastAxis) {
    
    assert(fromIndicies.size() > 0);
    // out params for the splitter. Note they are only defined if shouldSplit == true
    unsigned int splitAxis; 
    float leftMax, rightMin;

    bool shouldSplit=Splitter::GetSplit(triangles, bvh, node, fromIndicies, lastAxis, splitAxis, leftMax, rightMin);

    // are we creating a leaf?
    if(!shouldSplit) {
        // ok, leafy time.

        // we're going to append the indicies to the central array, and just remember the start+count in this node. 
        node.leftFirst = bvh.indicies.size();
        // give this node a count, which by definition makes it a leaf
        node.count = fromIndicies.size();
        // glue our index set on the back...
        for(unsigned int idx : fromIndicies) 
            bvh.indicies.push_back(idx);

        assert(node.isLeaf());
        assert(node.leftFirst == node.first());

        calcAABBIndirect(node.bounds, triangles, bvh.indicies, node.first(), node.count);
        std::cout << "leaf AABB " << node.bounds << " count " << node.count << std::endl;
    } else {
        // not creating a leaf, we're splitting - create a subdivision
       
        // we can't split a single triangle - shouldn't have recursed this far.
        // it's arguable we should have never gotten this deep (3's a good limit)
        assert(fromIndicies.size() > 1);
        assert(splitAxis < 3);

        // alloc child nodes
        node.leftFirst = bvh.nextFree;
        BVHNode& left = bvh.allocNextNode();
        BVHNode& right = bvh.allocNextNode();

        std::cout << "subdiv total count "<< fromIndicies.size() << " axis " << splitAxis; 
        std::cout<< " leftMax " << leftMax << " rightMin " << rightMin << std::endl;

        // walk the index array, build left and right sides.
        TriangleMapping leftIndicies, rightIndicies;
        for(unsigned int i = 0; i < fromIndicies.size(); i++) {
            unsigned int idx = fromIndicies[i];
            float valMin = triangles[idx].getMinCoord(splitAxis);
            float valMax = triangles[idx].getMaxCoord(splitAxis);
            
            std::cout << "idx " << idx;
            std::cout << " valMin " << valMin << " valMax " << valMax;
            std::cout << " leftMax " << leftMax << " rightMin " << rightMin << std::endl;;
            // FIXME: beware of >= or <= cases (hairy with floats)
            // .. we could miss triangles here
            if(valMin < leftMax)
                leftIndicies.push_back(idx);
            if(valMax > rightMin)
                rightIndicies.push_back(idx);
        }

        std::cout << " leftcount " << leftIndicies.size();
        std::cout << " rightCount " << rightIndicies.size() ;
        std::cout << std::endl;

        // if either of these fire, we've not 'split', we've put all the triangles on one side
        // (in which case, it's either a bad split value, or we should have created a leaf)
        assert(leftIndicies.size() > 0);
        assert(rightIndicies.size() > 0);

        // left and right can't contain more triangles than we were supplied with
        assert(leftIndicies.size() <= fromIndicies.size());
        assert(rightIndicies.size() <= fromIndicies.size());

        // recurse
        subdivide<Splitter>(triangles, bvh, left, leftIndicies, splitAxis);
        subdivide<Splitter>(triangles, bvh, right, rightIndicies, splitAxis);

        // now subdivide's done, combine aabb 
        combineAABB(node.bounds, left.bounds, right.bounds);
    }
}

template<class Splitter>
inline BVH* buildBVH(Scene& s) {
    BVH* bvh = new BVH(s.primitives.triangles.size());

    // setup "from" index map
    TriangleMapping indicies(s.primitives.triangles.size());
    for (unsigned int i = 0; i < s.primitives.triangles.size(); i++)
        indicies[i] = i;

    // recurse and subdivide
    subdivide<Splitter>(s.primitives.pos, *bvh, bvh->root(), indicies, 2);
    return bvh;
}

// Build a bad, but valid, BVH. This will have a single root node containing all triangles,
// so traversing it will degrade to a linear search. 
// Useful for testing worst case scenarios, or feeling bad about yourself.
struct StupidSplitter {
    static bool GetSplit(
            TrianglePosSet const& triangles,// in: master triangle array
            BVH& bvh,                       // in: BVH root 
            BVHNode& node,                  // in: current bvh node (to split)
            TriangleMapping const& indicies,// in: set of triangle indicies to split 
            unsigned int lastAxis,          // in: the axis on which the parent was split
            unsigned int& axis,             // out: axis on which to split
            float& leftMax,                 // out: max point to include in left set 
            float& rightMin) {              // out: min point to include in right set

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
            TrianglePosSet const& triangles,   
            BVH& bvh,                       
            BVHNode& node,                  
            TriangleMapping const& indicies,
            unsigned int lastAxis,
            unsigned int& axis,             
            float& leftMax, 
            float& rightMin) {   
        if(indicies.size() <= 3)
            return false; // don't split

        glm::vec3 total;
        glm::vec3 min(INFINITY, INFINITY, INFINITY);
        glm::vec3 max(-INFINITY, -INFINITY, -INFINITY);

        for (unsigned int i = 0; i < indicies.size() ; i++) {
            unsigned int index = indicies[i];
            TrianglePosition const& t = triangles[index];
            auto centroid = t.getCentroid();

            total += centroid;
            // FIXME: is there a glm func to do this (glm::min/max return the whole vec, not piecewise)
            min = glm::vec3(std::min(min.x,centroid.x), std::min(min.y,centroid.y),std::min(min.z,centroid.z));
            max = glm::vec3(std::max(max.x,centroid.x), std::max(max.y,centroid.y),std::max(max.z,centroid.z));
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
        leftMax = rightMin = total[axis] * (1.0f/indicies.size());

        return true; // do split
    }
};

// this one is not much better, but does subdivide.
inline BVH* buildMedianBVH(Scene& s) {
    std::cout << "building median BVH" << std::endl;
    BVH* bvh = buildBVH<AverageSplitter>(s);
    return bvh;
}

// Surface Area Heuristic splitter
struct SAHSplitter{
    static bool GetSplit(
            TrianglePosSet const& triangles,   
            BVH& bvh,                       
            BVHNode& node,                  
            TriangleMapping const& indicies,
            unsigned int lastAxis,
            unsigned int& axis,             
            float& leftMax, 
            float& rightMin) {   
        if(indicies.size() <= 3)
            return false;

        // size of the aabb
        glm::vec3 diff = node.bounds.high - node.bounds.low;

        // find biggest axis and write to OUT
        int const biggestAxis = diff[0]>diff[1] ? 
                                diff[0]>diff[2] ? 0 : 2
                              : diff[1]>diff[2] ? 1 : 2;
        axis = biggestAxis; // write to OUT

        // try a few places to split and find the one resulting in the smallest area
        float smallest_area_so_far=INFINITY;
        for(int i=1; i<8; i++){ // for each split
            float trySplitPoint = node.bounds.low[axis] + (i*(diff[axis]/8.f));
            // we keep a running total of the size of the left and right bounding box
            AABB left = {{INFINITY,INFINITY,INFINITY},{-INFINITY,-INFINITY,-INFINITY}};
            AABB right= {{INFINITY,INFINITY,INFINITY},{-INFINITY,-INFINITY,-INFINITY}};
            for(unsigned int t = 0; t<indicies.size(); t++){ // for each triangle
                unsigned int index = bvh.indicies[t];
                for(int v=0; v<3; v++){ // for each vertex
                    auto const& p = triangles[index].v[v];
                    if(p[biggestAxis] < trySplitPoint){
                        left.low = glm::min(left.low,p);
                        left.high= glm::max(left.high,p);
                    }else{
                        right.low = glm::min(right.low,p);
                        right.high= glm::max(right.high,p);
                    }
                }
            }
            float area = surfaceAreaAABB(left.high-left.low)
                       + surfaceAreaAABB(right.high-right.low);
            if(area<smallest_area_so_far){
                smallest_area_so_far = area;
                leftMax = rightMin = trySplitPoint; // write to OUT
            }
        }

        // TODO
        // now that we have found a split point, partition the trianges along that axis

        return true;
    }
};

BVH* buildSAHBVH(Scene& s){
    std::cout << "building SAH BVH" << std::endl;
    BVH* bvh = buildBVH<SAHSplitter>(s);
    return bvh;
}

inline BVH* buildBVH(Scene& s, BVHMethod method) {
    BVH* bvh = nullptr;

    switch(method) {
        case BVHMethod_STUPID: bvh = buildStupidBVH(s); break;
        case BVHMethod_MEDIAN: bvh = buildMedianBVH(s); break;
        case BVHMethod_SAH:    bvh = buildSAHBVH(s);    break;
        case __BVHMethod_MAX: assert(false); break; // shouldn't happen
    };

    std::cout << "bvh node count " << bvh->nextFree << std::endl;
    std::cout << "triangle count " << s.primitives.triangles.size() << std::endl;
    std::cout << "root AABB " << bvh->root().bounds << std::endl;

    sanityCheckBVH(*bvh, s.primitives.pos);

    return bvh;
}
