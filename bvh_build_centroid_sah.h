#pragma once

#include "aabb.h"
#include "bvh.h"
#include <array>

// this splitter will create a 'standard' BVH using the Surface Area Heuristic to split on centroids
// each triangle will be placed in a single leaf - ie it will not duplicate triangles in the BVH
struct CentroidSAHSplitter {
    // max number of slices (buckets) to test when splitting
    static constexpr int SAH_MAX_SLICES = 8;

    // Slice (or bucket) used when trying a triangle split
    struct Slice{
        Slice() : count (0) {}

        AABB aabb;
        int count;
    };

    static bool GetSplit(
            TrianglePosSet const& triangles,  // in: master triangle array
            TriangleMapping const& indicies,  // in: set of triangle indicies to split 
            AABB const& bounds,               // in: bounds of this set of triangles
            TriangleMapping& leftIndicies,    // out: resultant left set
            TriangleMapping& rightIndicies) { // out: resultant right set

        bounds.sanityCheck();

        if(indicies.size() <= 3) 
            return false;

        // get an AABB around all triangle centroids
        const AABB centroidBounds = buildAABBCentroid(triangles, indicies, 0, indicies.size());

        // bounds of centroids must be within total triangle bounds
        centroidBounds.sanityCheck();

        // centroid bounds must be inside triangle extrema bounds
        assert(containsAABB(bounds, centroidBounds));

        // find longest axis
        unsigned int axis = centroidBounds.longestAxis();

        // slice parent bounding box into slices along the longest axis
        // and count the triangle centroids in it
        std::array<Slice, SAH_MAX_SLICES> slices;

        // for chosen Axis, get high/low coords
        const float low = centroidBounds.low[axis];
        const float high = centroidBounds.high[axis];
        assert(low < high);
        const float sliceWidth = high - low;

        for(int const idx : indicies){
            const TrianglePos& tri = triangles[idx];
            
            // drop this centroid into a slice
            const float pos = tri.getAverageCoord(axis);
            const float ratio = ((pos - low) / sliceWidth);
            unsigned int sliceNo = ratio * SAH_MAX_SLICES;

            if(sliceNo == SAH_MAX_SLICES)
                sliceNo--;

            const AABB triBounds = triangleBounds(tri);

            slices[sliceNo].aabb = unionAABB(slices[sliceNo].aabb, triBounds);
            slices[sliceNo].count++;
        }

        // calculate cost after each slice
        const float boundingArea = surfaceAreaAABB(bounds);
        std::array<float, SAH_MAX_SLICES-1> costs;

        for(unsigned int i = 0; i < costs.size(); i++) {
            // glue slices together into a left slice and a right slice
            Slice left; 
            for(unsigned int j = 0; j <= i; j++){
                left.aabb = unionAABB(left.aabb, slices[j].aabb);
                left.count += slices[j].count;
            }

            Slice right;
            for(unsigned int j = i+1; j < slices.size(); j++){
                right.aabb = unionAABB(right.aabb, slices[j].aabb);
                right.count += slices[j].count;
            }

            float al = surfaceAreaAABB(left.aabb);
            float ar = surfaceAreaAABB(right.aabb);

            costs[i] = 1 + (left.count * al + right.count * ar) / boundingArea;
        }

        // find minimal permutation
        unsigned int splitSliceNo = 0;
        float minCost = costs[0];
        for(unsigned int i = 1; i < costs.size(); i++){
            if(costs[i] < minCost) {
                splitSliceNo = i;
                minCost = costs[i];
            }
        }

        // check termination heurisic...
        if(minCost > indicies.size()) {
            return false; // no splitting here, chopper
        }

        // ok, we're going to split. parition the indicies based on bucket
        for(unsigned int idx : indicies) {
            // determine slice in which this one belongs
            float val = triangles[idx].getAverageCoord(axis);
            const float ratio = ((val - low) / sliceWidth);
            unsigned int sliceNo = ratio * SAH_MAX_SLICES;
            if(sliceNo == SAH_MAX_SLICES)
                sliceNo--;

            if(sliceNo <= splitSliceNo)
                leftIndicies.push_back(idx);
            else
                rightIndicies.push_back(idx);
        }

        // make sure all triangles are accounted for. we don't make duplicate triangles, 
        // so all tris should be on one side only
        assert(leftIndicies.size() + rightIndicies.size() == indicies.size());
        return true; // yes, we split!
    }
};

inline BVH* buildCentroidSAHBVH(Scene& s) {
    std::cout << "building centroid SAH BVH" << std::endl;
    BVH* bvh = buildBVH<CentroidSAHSplitter>(s);
    return bvh;
}

