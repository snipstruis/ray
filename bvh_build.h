#pragma once

#include "bvh.h"
#include "scene.h"

enum BVHMethod {
    BVHMethod_STUPID,
    BVHMethod_SAH,
    __BVHMethod_MAX
};

const char* BVHMethodStr(BVHMethod m) {
    switch(m) {
        case BVHMethod_STUPID: return "STUPID";
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

    // the set of triangles in this node is already known, so calculate the bounds now before calling the
    calcAABBIndirect(node.bounds, triangles, fromIndicies, 0, fromIndicies.size());

    // out params for the splitter. Note they are only defined if shouldSplit == true
    unsigned int splitAxis; 
    float leftMax, rightMin;

    bool shouldSplit=Splitter::GetSplit(triangles, fromIndicies, node.bounds, lastAxis, splitAxis, leftMax,rightMin);

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
            // We want to include the triangle if at least one part of it is within the range
            if(valMin <= leftMax)
                leftIndicies.push_back(idx);
            if(valMax >= rightMin)
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
        // note that this is <, not <=. if either child contains all the triangles,
        // we don't have a stopping condition, and we'll recurse forever. and the
        // universe hates this.
        assert(leftIndicies.size() < fromIndicies.size());
        assert(rightIndicies.size() < fromIndicies.size());

        // recurse
        subdivide<Splitter>(triangles, bvh, left, leftIndicies, splitAxis);
        subdivide<Splitter>(triangles, bvh, right, rightIndicies, splitAxis);

        // now the child node should be set up, its indicies should 
        // be valid
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
            TriangleMapping const& indicies,// in: set of triangle indicies to split 
            AABB const& bounds,             // in: bounds of this set of triangles
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

// Surface Area Heuristic splitter
struct SAHSplitter{
    static bool GetSplit(
            TrianglePosSet const& triangles,   
            TriangleMapping const& indicies,
            AABB const& bounds,
            unsigned int lastAxis,
            unsigned int& splitAxis,             
            float& leftMax, 
            float& rightMin) {   
        // size of the aabb
        glm::vec3 diff = bounds.high - bounds.low;

        // try a few places to split and find the one resulting in the smallest area
        float smallest_area_so_far = INFINITY;
        bool split_good_enough = false;
        for(int axis=0; axis<3; axis++) for(int split=1; split<8; split++){
            float trySplitPoint = bounds.low[axis] + (split*(diff[axis]/8.f));
            // we keep a running total of the size of the left and right bounding box
            AABB left = {{INFINITY,INFINITY,INFINITY},{-INFINITY,-INFINITY,-INFINITY}};
            AABB right= {{INFINITY,INFINITY,INFINITY},{-INFINITY,-INFINITY,-INFINITY}};
            float triangles_in_left  = 0;
            float triangles_in_right = 0;
            for(int t = 0; t < indicies.size(); t++){ // for each triangle
                TrianglePosition const& triangle = triangles[indicies[t]];
                // find out if the triangle belongs to left, right or both ...
                bool in_left = false;
                bool in_right= false;
                for(int vertex=0; vertex<3; vertex++){
                    glm::vec3 p = triangle.v[vertex];
                    if(p[axis] < trySplitPoint) in_left = true; else in_right = true;
                }
                // ... then grow the bounding boxes if it contains the triangle
                if(in_left){
                    for(int vertex=0; vertex<3; vertex++){
                        glm::vec3 p = triangle.v[vertex];
                        left.low = glm::min(left.low,p);
                        left.high= glm::max(left.high,p);
                        triangles_in_left++;
                    }
                }
                if(in_right){
                    for(int vertex=0; vertex<3; vertex++){
                        glm::vec3 p = triangle.v[vertex];
                        right.low = glm::min(right.low,p);
                        right.high= glm::max(right.high,p);
                        triangles_in_right++;
                    }
                }
            }
            // now check if the split is the best so far
            float al = surfaceAreaAABB(left);
            float ar = surfaceAreaAABB(right);
            float area= al+ar;
            if(area<smallest_area_so_far){
                smallest_area_so_far = area;
                // if it is the best so far, check if we should split at all
                float triangle_count = triangles_in_right+triangles_in_left;
                if((al*triangles_in_left + ar*triangles_in_right)<(area*triangle_count)){
                    // we should split, set all the output variables
                    split_good_enough = true;
                    leftMax  = fmax(left.low[axis], right.low[axis]);
                    rightMin = fmin(left.high[axis],right.high[axis]);
                    splitAxis  = axis;
                }
            }
        }

        return split_good_enough;
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
        case BVHMethod_SAH:    bvh = buildSAHBVH(s);    break;
        case __BVHMethod_MAX: assert(false); break; // shouldn't happen
    };

    std::cout << "bvh node count " << bvh->nextFree << std::endl;
    std::cout << "triangle count " << s.primitives.triangles.size() << std::endl;
    std::cout << "root AABB " << bvh->root().bounds << std::endl;

    sanityCheckBVH(*bvh, s.primitives.pos);

    return bvh;
}
