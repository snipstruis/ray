#pragma once

// max number of slices (buckets) to test when splitting
constexpr int SLICES_PER_AXIS = 8;
// alpha value used with excluding spatial splits per section 4.5 of the paper
constexpr float SBVH_ALPHA = 10e-8f;

enum SplitKind {
    OBJECT,
    SPATIAL
};

struct SplitDecision {
    SplitDecision() : 
        minCost(INFINITY), 
        surfaceArea(0.0f),
        chosenSplitNo(-1), 
        chosenAxis(-1), 
        splitKind(OBJECT), 
        leftCount(0), 
        rightCount(0) 
    {}

    SplitDecision(float cost, int splitNo, int axis, SplitKind kind, int _leftCount , int _rightCount) : 
        minCost(cost), 
        surfaceArea(0.0f),
        chosenSplitNo(splitNo), 
        chosenAxis(axis), 
        splitKind(kind),
        leftCount(_leftCount), 
        rightCount(_rightCount) 
    {}

    SplitDecision(float cost, float _surfaceArea, int splitNo, int axis, SplitKind kind) :
        minCost(cost), 
        surfaceArea(_surfaceArea),
        chosenSplitNo(splitNo), 
        chosenAxis(axis), 
        splitKind(kind),
        leftCount(0), 
        rightCount(0) 
    {}

    // once the dust has settled, check we've got a result
    void sanityCheck() const {
        assert(minCost < INFINITY);
        assert(chosenSplitNo >= 0);
        assert(chosenSplitNo < SLICES_PER_AXIS);
        assert(chosenAxis >= 0);
        assert(chosenAxis < 3);
        assert(leftCount >= 0);
        assert(rightCount >= 0);
    }

    void merge(SplitDecision const& other) {
        other.sanityCheck();

        if(other.minCost < minCost) {
            minCost = other.minCost;
            surfaceArea = other.surfaceArea;
            chosenSplitNo = other.chosenSplitNo;
            chosenAxis = other.chosenAxis;
            splitKind = other.splitKind;
            leftCount = other.leftCount;
            rightCount = other.rightCount;
        }
    }

    // lowest cost we've seen so far
    float minCost;
    // surface area of the union of the 2x bounding boxes for an object split
    float surfaceArea;
    // for this minCost - 
    int chosenSplitNo;
    int chosenAxis;
    SplitKind splitKind;
    // for spatial splits, we remember the counts on either side of the split point, 
    // we use this when unsplitting
    int leftCount;
    int rightCount;
    
};

std::ostream& operator<<(std::ostream& os, const SplitDecision& d) {
    os << " minCost " << d.minCost;
    os << " chosenSplitNo " << d.chosenSplitNo;
    os << " chosenAxis " << d.chosenAxis;
    os << " splitKind " << (d.splitKind == OBJECT ? "OBJECT" : "SPATIAL");
    os << " leftCount " << d.leftCount;
    os << " rightCount " << d.rightCount;
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
            decision.merge(SplitDecision(cost, i, axis, kind, left.count, right.count));
        }
    }

    // parition triangles in a spatial split
    static bool DoSpatialSplit(
            SplitDecision const& decision,
            AABB const& extremaBounds,
            TrianglePosSet const& triangles,  // in: master triangle array
            TriangleMapping const& indicies,  // in: set of triangle indicies to split 
            TriangleMapping& leftIndicies,    // out: resultant left set
            TriangleMapping& rightIndicies) { // out: resultant right set

        assert(decision.splitKind == SPATIAL);
        assert(decision.chosenSplitNo < (SLICES_PER_AXIS - 1));

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

        // for the purposes of reference-unsplitting, calc the bounds on both sides of the split
        AABB boundsLeft, boundsRight;

        for(unsigned int idx : indicies) {
            // determine slice in which this one belongs
            TrianglePos const& tri = triangles[idx];
            if(tri.getMinCoord(decision.chosenAxis) <= splitPoint) {
                boundsLeft = unionTriangle(boundsLeft, tri);
            }

            if(tri.getMaxCoord(decision.chosenAxis) >= splitPoint) {
                boundsRight = unionTriangle(boundsRight, tri);
            }
        }

        float areaLeft = surfaceAreaAABB(boundsLeft);
        float areaRight = surfaceAreaAABB(boundsRight);

        float cSplit = (areaLeft * decision.leftCount) + (areaRight * decision.rightCount);

        assert(areaLeft <= surfaceAreaAABB(extremaBounds));
        assert(areaRight <= surfaceAreaAABB(extremaBounds));

        // ok, we're going to split. parition the indicies based on bucket
        for(unsigned int idx : indicies) {
            // determine slice in which this one belongs
            TrianglePos const& tri = triangles[idx];
            float minCoord = tri.getMinCoord(decision.chosenAxis); 
            float maxCoord = tri.getMaxCoord(decision.chosenAxis); 

            // firstly, let's check if this is a split reference - ie the triangle spans the split
            // point.. if so, we'll do the 'unsplitting' test
            
            if(minCoord <= splitPoint && maxCoord >= splitPoint) { 
                //  unsplitting: get the union bounds on both sides of the split
                AABB unionLeft = unionTriangle(boundsLeft, tri);
                AABB unionRight = unionTriangle(boundsRight, tri);

                // and the costs for going left or right
                float cL = (surfaceAreaAABB(unionLeft)*decision.leftCount) + (areaRight*(decision.rightCount-1));
                float cR = (areaLeft*(decision.leftCount-1)) + (surfaceAreaAABB(unionRight)*decision.rightCount);

                // go the cheapest option
                if(cSplit < cL && cSplit < cR) {
                    leftIndicies.push_back(idx);
                    rightIndicies.push_back(idx);
                } else if (cL < cR) {
                    leftIndicies.push_back(idx);
                } else { // cR is cheapest
                    rightIndicies.push_back(idx);
                }
            } else {
                // triangle is not split - just thwack it one one side
                if(minCoord < splitPoint) {
                    assert(maxCoord < splitPoint);
                    leftIndicies.push_back(idx);
                } else {
                    assert(maxCoord > splitPoint);
                    rightIndicies.push_back(idx);
                }
            }
        }

        assert(leftIndicies.size() + rightIndicies.size() >= indicies.size());
        assert(leftIndicies.size() <= indicies.size());
        assert(rightIndicies.size() <= indicies.size());

        // so this sucks. 
        // We've ended up putting all triangles in (at least) one side of the split. 
        // If we do nothing here, we'll just recurse forever, trying the same split again and again.
        // So, undo our work, and return false. We'll then fall back on the best object split we've got.
        if(leftIndicies.size() == indicies.size() || rightIndicies.size() == indicies.size()) {
            leftIndicies.clear();
            rightIndicies.clear();
            return false;
        }

        return true;
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
            unsigned int sliceNo = (unsigned int)(ratio * SLICES_PER_AXIS);

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
            
            float cLeft = left.count * areaLeft;
            float cRight = right.count * areaRight;
            float cost = 1 + ((cLeft + cRight) / boundingSurfaceArea);

            float unionSurfaceArea = surfaceAreaAABB(unionAABB(left.bounds, right.bounds));
            SplitDecision result(cost, unionSurfaceArea, i, axis, kind);
            decision.merge(result);
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

        assert(decision.splitKind == OBJECT);

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
            int sliceNo = (unsigned int)(ratio * SLICES_PER_AXIS);
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

        SplitDecision bestSpatial;

        // walk the 3 axis, find the lowest cost split
        // ... give spatial splits a go.
        for(int axis = 0; axis < 3; axis++) {
            TrySpatialSplits(triangles, indicies, boundingArea, extremaBounds, axis, bestSpatial);
        }

        bestSpatial.sanityCheck();

        SplitDecision bestObject;

        // ... try object splits
        for(int axis = 0; axis < 3; axis++) { 
            TryObjectSplits(triangles, indicies, boundingArea, centroidBounds, axis, bestObject);
        }

        bestObject.sanityCheck();

        // find ultimate best split decision..
        SplitDecision best;
        best.merge(bestSpatial);
        best.merge(bestObject);

        // check termination heurisic...
        if(best.minCost > indicies.size()) {
            return false; // no splitting here, chopper. make a leaf with this triangle set
        }

        // okeydokes, we're going to split this node.. object or spatial split?
        if(best.splitKind == SPATIAL) {
            // okeydokes, we're attempting a spatial split...
            // perform the 'restricting spatial split attempts' check from part 4.5 of the SBVH paper
            // the best spatial case should be populated with the union bounding box area - 
            // compare it to the root bounds.
        
            // root bounds should be initialised by now...
            bvh.root().bounds.sanityCheck();
            float rootBoundsArea = surfaceAreaAABB(bvh.root().bounds);
            assert(bestObject.surfaceArea > 0.0f);
            float ratio = bestObject.surfaceArea / rootBoundsArea;

            if(ratio > SBVH_ALPHA) {
                if(DoSpatialSplit(best, extremaBounds, triangles, indicies, leftIndicies, rightIndicies)) {
                    bvh.spatialSplits++;
                    return true;
                }
            }
        }

        bvh.objectSplits++;

        DoObjectSplit(bestObject, centroidBounds, triangles, indicies, leftIndicies, rightIndicies);

        return true; // yes, we split!
    }
};

BVH* buildSBVH(Scene& s){
    std::cout << "building SBVH" << std::endl;
    BVH* bvh = buildBVH<SBVHSplitter>(s);
    return bvh;
}

