#pragma once

// max number of slices (buckets) to test when splitting
constexpr int SLICES_PER_AXIS = 8;
constexpr float SAH_ALPHA = 10e-15;

enum SplitKind {
    OBJECT,
    SPATIAL
};

struct SplitDecision {
    SplitDecision(): minCost(INFINITY), chosenSplitNo(-1), chosenAxis(-1), splitKind(OBJECT) {}

    SplitDecision(float cost, int splitNo, int axis, SplitKind kind): 
        minCost(cost), chosenSplitNo(splitNo), chosenAxis(axis), splitKind(kind) {}

    // once the dust has settled, check we've got a result
    void sanityCheck() const {
        assert(minCost < INFINITY);
        assert(chosenSplitNo >= 0);
        assert(chosenSplitNo < SLICES_PER_AXIS);
        assert(chosenAxis >= 0);
        assert(chosenAxis < 3);
    }

    void merge(SplitDecision const& other) {
        other.sanityCheck();

        if(other.minCost < minCost) {
            minCost = other.minCost;
            chosenSplitNo = other.chosenSplitNo;
            chosenAxis = other.chosenAxis;
            splitKind = other.splitKind;
        }
    }

    // lowest cost we've seen so far
    float minCost;
    // for this minCost - 
    int chosenSplitNo;
    int chosenAxis;
    SplitKind splitKind;
};

std::ostream& operator<<(std::ostream& os, const SplitDecision& d) {
    os << " minCost " << d.minCost;
    os << " chosenSplitNo " << d.chosenSplitNo;
    os << " chosenAxis " << d.chosenAxis;
    os << " splitKind " << (d.splitKind == OBJECT ? "OBJECT" : "SPATIAL");
    return os;
}

// build an SBVH - that is a Split Bounding Volume Hierachy
struct SBVHSplitter {

    // Slice (or bucket) used when trying an Object split
    struct Slice{
        Slice() : count(0) {}

        AABB bounds;
        int count;
    };

    typedef std::array<Slice, SLICES_PER_AXIS> SliceArray;

    // Slice used when trying a Spatial split
    struct SpatialSlice{
        SpatialSlice() : entryCount(0), exitCount(0) {}

        AABB bounds;
        int entryCount, exitCount;
    };

    typedef std::array<SpatialSlice, SLICES_PER_AXIS> SpatialSliceArray;

    // tries an SAH Spatial split on the given axis. 
    // may be a no-op if the given axis is zero length
    static void TrySpatialSplits(
            TrianglePosSet const& triangles,  // in: master triangle array
            TriangleMapping const& indicies,  // in: set of triangle indicies to split 
            float boundingSurfaceArea,        // in: surface area of extrema bounding box
            AABB const& extremaBounds,        // in: bounds of this set of triangles
            int axis,                         // in: axis to test
            SplitDecision& decision) {        // out: resultant decision

        assert(indicies.size() > 1);

        // slice parent bounding box into slices along the longest axis
        // and count the triangle centroids in it
        SpatialSliceArray slices;

        const float low = extremaBounds.low[axis];
        const float high = extremaBounds.high[axis];

        assert(low <= high);

        // it's possible to get a case where low==high. just ignore this axis
        if(!(low < high))
            return;

        const float range = high - low;
        const float sliceWidth = range / (float)SLICES_PER_AXIS;

        // walk all triangles
        for(int const idx : indicies) {
            const TrianglePos& tri = triangles[idx];

            // walk across the slices
            for(int sliceNo = 0; sliceNo < SLICES_PER_AXIS; sliceNo++) {
                float sliceLow = ((float)sliceNo * sliceWidth) + low;
                float sliceHigh = sliceLow + sliceWidth;
                assert(sliceHigh <= (high + EPSILON));

                // does this tri fall in this slice?
                // note we consider an == to be a miss, as it means one extreme coord is sitting on 
                // the boundary rather than clipping it
                if(tri.getMinCoord(axis) >= sliceHigh || tri.getMaxCoord(axis) <= sliceLow) {
                    continue;
                }
            
                auto& slice = slices[sliceNo];

                bool clippedLow = false;

                // tri clips low side of slice?
                if(tri.getMinCoord(axis) < sliceLow) {
                    VecPair intersect;
                    clipTriangle(tri, axis, sliceLow, intersect);
                    slice.bounds = unionVecPair(slice.bounds, intersect);
                    clippedLow = true;
                } else {
                    // doesn't clip low side - tri must start in this slice
                    slice.entryCount++;
                }

                bool clippedHigh = false;

                // tri clips high side of slice?
                if(tri.getMaxCoord(axis) > sliceHigh) {
                    VecPair intersect;
                    clipTriangle(tri, axis, sliceHigh, intersect);
                    slice.bounds = unionVecPair(slice.bounds, intersect);
                    clippedHigh = true;
                } else {
                    // doesn't clip high side - tri must end in this slice
                    slice.exitCount++;
                }

                // in the case where a triangle doesn't clip both sides - that is, it starts, ends
                // or is fully contained in this slice, we need to grow bounds to contain the verticies
                // are within this slice. strictly speaking, this outer test is redundant, but we've
                // already tested this once, might as well take advantage (yes, i know, more branchy code is bad)
                // sigh
                if(!clippedLow || !clippedHigh) {
                    int num = 0;
                    for(int i = 0; i < 3; i++) {
                        float val = tri.v[i][axis];
                        // note we do >= and <= here, as we earlier excluded points that sit exactly on the 
                        // split point. so take care of them now.
                        if(val <= sliceHigh || val >= sliceLow) {
                            slice.bounds = unionPoint(slice.bounds, tri.v[i]);
                            num++;
                        }
                    }
                    // we must have seen at least one point here.
                    assert(num > 0);
                }

            }
        }
            
        FindSpatialMinCostSplit(slices, boundingSurfaceArea, axis, SPATIAL, decision);
    }

    // calculate cost after slicing
    static void FindSpatialMinCostSplit(
            SpatialSliceArray const& slices, // in: slices to consider
            float boundingSurfaceArea,       // in: surface area of extrema bounding box
            int axis,                        // in: axis we're splitting on
            SplitKind kind,                  // in: kind of split we're performing (for stats purposes)
            SplitDecision& decision) {       // in/out: resultant decision

        for(unsigned int i = 0; i < (slices.size() - 1) ; i++) {
            // glue slices together into a left & right total cost
            Slice left, right;

            for(unsigned int j = 0; j <= i; j++){
                left.bounds = unionAABB(left.bounds, slices[j].bounds);
                left.count += slices[j].entryCount;
            }

            for(unsigned int j = i+1; j < slices.size(); j++){
                right.bounds = unionAABB(right.bounds, slices[j].bounds);
                right.count += slices[j].exitCount;
            }

            if(left.count == 0 && right.count == 0)
                continue;
            //assert(leftCount > 0);
            //assert(rightCount > 0);

            float areaLeft = 0.0f;
            float areaRight = 0.0f;

            if(left.count > 0) {
                left.bounds.sanityCheck();
                areaLeft = surfaceAreaAABB(left.bounds);
            }

            if(right.count > 0) {
                right.bounds.sanityCheck();
                areaRight = surfaceAreaAABB(right.bounds);
            }
            
            float cost = (1 + (left.count * areaLeft + right.count * areaRight) / boundingSurfaceArea);
            decision.merge(SplitDecision(cost, i, axis, kind));
        }
    }

    // parition triangles in a spatial split
    static void DoSpatialSplit(
            SplitDecision const& decision,
            AABB const& extremaBounds,
            TrianglePosSet const& triangles,  // in: master triangle array
            TriangleMapping const& indicies,  // in: set of triangle indicies to split 
            TriangleMapping& leftIndicies,    // out: resultant left set
            TriangleMapping& rightIndicies) { // out: resultant right set

        const float low = extremaBounds.low[decision.chosenAxis];
        const float high = extremaBounds.high[decision.chosenAxis];
        
        // at this point, low must be < high (ie not equal) as we've chosen it as a split axis
        // this means there must be a point in this axis we can split the triangles
        assert(low < high);
        const float range = high - low;
        const float sliceWidth = range / (float)SLICES_PER_AXIS;

        float splitPoint = ((decision.chosenSplitNo + 1) * sliceWidth) + low;

        assert(splitPoint < high);
        assert(splitPoint > low);
        assert(decision.chosenSplitNo < (SLICES_PER_AXIS - 1));

        // ok, we're going to split. parition the indicies based on bucket
        for(unsigned int idx : indicies) {
            // determine slice in which this one belongs
            TrianglePos const& tri = triangles[idx];

            if(tri.getMinCoord(decision.chosenAxis) <= splitPoint) {
                leftIndicies.push_back(idx);
            }

            if(tri.getMaxCoord(decision.chosenAxis) >= splitPoint) {
                rightIndicies.push_back(idx);
            }
        }

        assert(leftIndicies.size() + rightIndicies.size() >= indicies.size());
        assert(leftIndicies.size() <= indicies.size());
        assert(rightIndicies.size() <= indicies.size());
    }

    // tries an SAH Object split on the given axis.
    // may be a no-op if the given axis is zero length
    static void TryObjectSplits(
            TrianglePosSet const& triangles,  // in: master triangle array
            TriangleMapping const& indicies,  // in: set of triangle indicies to split 
            float boundingSurfaceArea,        // in: surface area of extrema bounding box
            AABB const& centroidBounds,       // in: bounds of this set of triangles
            int axis,                         // in: axis to test
            SplitDecision& decision) {        // out: resultant decision
        assert(indicies.size() > 1);

        // slice parent bounding box into slices along the longest axis
        // and count the triangle centroids in it
        SliceArray slices;

        const float low = centroidBounds.low[axis];
        const float high = centroidBounds.high[axis];
        // it's possible to get a case where low==high. just ignore this axis
        if(!(low < high))
            return;

        const float range = high - low;

        for(int const idx : indicies) {
            const TrianglePos& tri = triangles[idx];
            
            // drop this centroid into a slice
            const float pos = tri.getAverageCoord(axis);
            const float ratio = ((pos - low) / range);
            unsigned int sliceNo = ratio * SLICES_PER_AXIS;

            if(sliceNo == SLICES_PER_AXIS)
                sliceNo--;

            const AABB triBounds = triangleBounds(tri);
            triBounds.sanityCheck();

            assert(sliceNo < slices.size());
            auto& slice = slices[sliceNo]; 

            slice.bounds = unionAABB(slices[sliceNo].bounds, triBounds);
            slice.bounds.sanityCheck();
            slice.count++;
        }

        FindObjectMinCostSplit(slices, boundingSurfaceArea, axis, OBJECT, decision);
    }

    // calculate cost after slicing
    static void FindObjectMinCostSplit(
            SliceArray const& slices,      // in: slices to consider
            float boundingSurfaceArea,     // in: surface area of extrema bounding box
            int axis,                      // in: axis we're splitting on
            SplitKind kind,                // in: kind of split we're performing (for stats purposes)
            SplitDecision& decision) {     // out: resultant decision

        for(auto const& slice : slices) {
            if(slice.count > 0) {
                slice.bounds.sanityCheck();
            }
        }

        for(unsigned int i = 0; i < (slices.size() - 1) ; i++) {
            // glue slices together into a left slice and a right slice
            Slice left, right;

            for(unsigned int j = 0; j <= i; j++){
                left.bounds = unionAABB(left.bounds, slices[j].bounds);
                left.count += slices[j].count;
            }

            for(unsigned int j = i+1; j < slices.size(); j++){
                right.bounds = unionAABB(right.bounds, slices[j].bounds);
                right.count += slices[j].count;
            }

            if(left.count == 0 && right.count == 0)
                continue;
            //assert(leftCount > 0);
            //assert(rightCount > 0);

            float areaLeft = 0.0f;
            float areaRight = 0.0f;

            if(left.count > 0) {
                left.bounds.sanityCheck();
                areaLeft = surfaceAreaAABB(left.bounds);
            }

            if(right.count > 0) {
                right.bounds.sanityCheck();
                areaRight = surfaceAreaAABB(right.bounds);
            }
            
            float cost = (1 + (left.count * areaLeft + right.count * areaRight) / boundingSurfaceArea);
            decision.merge(SplitDecision(cost, i, axis, kind));
        }
    }

    // parition triangles in an object split
    static void DoObjectSplit(
            SplitDecision const& decision,
            AABB const& centroidBounds,
            TrianglePosSet const& triangles,  // in: master triangle array
            TriangleMapping const& indicies,  // in: set of triangle indicies to split 
            TriangleMapping& leftIndicies,    // out: resultant left set
            TriangleMapping& rightIndicies) { // out: resultant right set

        const float low = centroidBounds.low[decision.chosenAxis];
        const float high = centroidBounds.high[decision.chosenAxis];
        
        // at this point, low must be < high (ie not equal) as we've chosen it as a split axis
        // this means there must be a point in this axis we can split the triangles
        assert(low < high);
        const float sliceWidth = high - low;

        // ok, we're going to split. parition the indicies based on bucket
        for(unsigned int idx : indicies) {
            // determine slice in which this one belongs
            const float val = triangles[idx].getAverageCoord(decision.chosenAxis);
            const float ratio = ((val - low) / sliceWidth);
            int sliceNo = ratio * SLICES_PER_AXIS;
            if(sliceNo == SLICES_PER_AXIS)
                sliceNo--;

            if(sliceNo <= decision.chosenSplitNo)
                leftIndicies.push_back(idx);
            else
                rightIndicies.push_back(idx);
        }

        // make sure all triangles are accounted for. we don't make duplicate triangles, 
        // so all tris should be on one side only
        assert(leftIndicies.size() + rightIndicies.size() == indicies.size());
    }

    // main splitter entry point
    static bool TrySplit(
            BVH& bvh,                         // in: bvh root
            TrianglePosSet const& triangles,  // in: master triangle array
            TriangleMapping const& indicies,  // in: set of triangle indicies to split 
            AABB const& extremaBounds,        // in: bounds of this set of triangles
            TriangleMapping& leftIndicies,    // out: resultant left set
            TriangleMapping& rightIndicies) { // out: resultant right set

        extremaBounds.sanityCheck();

        // force a leaf if we're only given 3 triangles.
        if(indicies.size() <= 3) 
            return false;

        const float boundingArea = surfaceAreaAABB(extremaBounds);

        // the bounds that we're given is an extrema brounds. We need one that surrounds the 
        // triangle centroids for object splits
        const AABB centroidBounds = buildAABBCentroid(triangles, indicies, 0, indicies.size());

        centroidBounds.sanityCheck();

        SplitDecision decision;

        // ... now give spatial splits a go.
        for(int axis = 0; axis < 3; axis++) {
            TrySpatialSplits(triangles, indicies, boundingArea, extremaBounds, axis, decision);
        }

        decision.sanityCheck();

        // walk the 3 axis, find the lowest cost split
        // ... firstly, try object splits
        for(int axis = 0; axis < 3; axis++) { 
            TryObjectSplits(triangles, indicies, boundingArea, centroidBounds, axis, decision);
        }

        decision.sanityCheck();


        // check termination heurisic...
        if(decision.minCost > indicies.size()) {
            return false; // no splitting here, chopper. make a leaf with this triangle set
        }

        // okeydokes, we're going to split this node.. object or spatial split?
        if(decision.splitKind == OBJECT) {
            bvh.objectSplits++;

            DoObjectSplit(decision, centroidBounds, triangles, indicies, leftIndicies, rightIndicies);
        } else {
            bvh.spatialSplits++;

            DoSpatialSplit(decision, extremaBounds, triangles, indicies, leftIndicies, rightIndicies);

            // so this sucks. If all the triangles land on one side, then don't split; this 
            // would mean we'll recurse forever.
            if(leftIndicies.size() == indicies.size() || rightIndicies.size() == indicies.size()) {
                bvh.spatialSplits--;
 //               std::cout << " -- sigh, nasty term"  << std::endl;
                return false;
            }
        }

        return true; // yes, we split!
    }
};

BVH* buildSBVH(Scene& s){
    std::cout << "building SBVH" << std::endl;
    BVH* bvh = buildBVH<SBVHSplitter>(s);
    return bvh;
}

