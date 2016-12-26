#pragma once

#include "aabb.h"
#include "bvh.h"
#include "bvh_diag.h"
#include "scene.h"
#include "timer.h"

#include <array>

// this is the common library for building BVHes - the specific builders are now in their own file


enum BVHMethod {
//    BVHMethod_STUPID,
    BVHMethod_CENTROID_SAH,
    BVHMethod_SBVH,
    __BVHMethod_MAX
};

const char* BVHMethodStr(BVHMethod m) {
    switch(m) {
 //       case BVHMethod_STUPID: return "STUPID";
        case BVHMethod_CENTROID_SAH: return "SAH";
        case BVHMethod_SBVH: return "SBVH";
        case __BVHMethod_MAX: return "shouldnt happen";
    };
}

// general purpose recursive BVH subdivider function
// @Splitter defines the particular constuction algorithm
template <class Splitter>
void subdivide(
        TrianglePosSet const& triangles, 
        BVH& bvh, 
        BVHNode& node, 
        TriangleMapping const& fromIndicies) {
    assert(fromIndicies.size() > 0);

//    std::cout << "subdiv total count " << fromIndicies.size();

    // the set of triangles in this node is already known, so calculate the bounds now before subdividing
    node.bounds = buildAABBExtrema(triangles, fromIndicies, 0, fromIndicies.size());

    TriangleMapping leftIndicies, rightIndicies;

    // call into the specific splitter function
    bool didSplit = Splitter::GetSplit(
            triangles, fromIndicies, node.bounds, leftIndicies, rightIndicies);

    // if the splitter didn't split, we are creating a leaf.
    if(!didSplit) {
        // ok, leafy time.

        // we're going to append the indicies to the central array, and just remember the 
        // start+count in this node. 
        node.leftFirst = bvh.indicies.size();
        // give this node a count, which by definition makes it a leaf
        node.count = fromIndicies.size();
        // glue our index set on the back...
        for(unsigned int idx : fromIndicies) 
            bvh.indicies.push_back(idx);

        assert(node.isLeaf());
        assert(node.leftFirst == node.first());

        //std::cout << "  leaf AABB " << node.bounds << std::endl;
    } else {
        // not creating a leaf, we've split
        // leftIndicies and rightIndicies should now be populated - we'll recurse and go again.
       
        // we can't have split a single triangle - shouldn't gotten this far.
        assert(fromIndicies.size() > 1);

#if 0
        // walk the index array, build left and right sides.
        std::cout << " leftCount " << leftIndicies.size();
        std::cout << " rightCount " << rightIndicies.size();
        std::cout << std::endl;
#endif

        // if either of these fire, we've not split, we've put all the triangles on one side
        // (in which case, it's either a bad split value, or we should have created a leaf)
        assert(leftIndicies.size() > 0);
        assert(rightIndicies.size() > 0);

        // check that all trianges are accounted for AT LEAST once. some splitters may duplicate 
        // triangles on both sides of the split, hence the >=
        // FIXME: really should walk fromIndicies, and check that all the tris are in at least one side
        assert(leftIndicies.size() + rightIndicies.size() >= fromIndicies.size());

        // left and right can't contain more triangles than we were supplied with
        // note that this is <, not <=. if either child contains all the triangles,
        // we don't have a stopping condition, and we'll recurse forever. and the
        // universe hates this.
        assert(leftIndicies.size() < fromIndicies.size());
        assert(rightIndicies.size() < fromIndicies.size());

        // alloc child nodes
        node.leftFirst = bvh.nextFree;
        BVHNode& left = bvh.allocNextNode();
        BVHNode& right = bvh.allocNextNode();

        // recurse
        subdivide<Splitter>(triangles, bvh, left, leftIndicies);
        subdivide<Splitter>(triangles, bvh, right, rightIndicies);
    }

    // now we're done, node should be fully setup. check
    assert(surfaceAreaAABB(node.bounds) > 0.0f);
    if(node.isLeaf()){
        assert((node.leftFirst + node.count) <= bvh.indicies.size());
    }
}

template<class Splitter>
inline BVH* buildBVH(Scene& s) {
    BVH* bvh = new BVH(s.primitives.pos.size());

    // setup "from" index map
    TriangleMapping indicies(s.primitives.extra.size());
    for (unsigned int i = 0; i < s.primitives.extra.size(); i++)
        indicies[i] = i;

    // recurse and subdivide
    subdivide<Splitter>(s.primitives.pos, *bvh, bvh->root(), indicies);
    return bvh;
}


