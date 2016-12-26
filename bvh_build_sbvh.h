#pragma once

// max number of slices (buckets) to test when splitting
static constexpr int SLICES_PER_AXIS = 8;

enum SplitKind {
    OBJECT,
    SPATIAL
};

struct SplitDecision {
    SplitDecision(): minCost(INFINITY), chosenSplitNo(-1), chosenAxis(-1), splitKind(OBJECT) {}

    // once the dust has settled, check we've got a result
    void sanityCheck() {
        assert(minCost < INFINITY);
        assert(chosenSplitNo >= 0);
        assert(chosenSplitNo < SLICES_PER_AXIS);
        assert(chosenAxis >= 0);
        assert(chosenAxis < 3);
    }

    void addCandidate(float cost, int splitNo, int axis, SplitKind kind) {
        // cost=INFINITY should already be eliminated at this point
        assert(cost < INFINITY);

#if 0
    std::cout  << "   cand: cost " << cost;
    std::cout  << " splitNo " << splitNo;
    std::cout  << " axis " << axis;
    std::cout  << " kind " << (kind == OBJECT ? "OBJECT" : "SPATIAL");
#endif

        if(cost < minCost) {
            minCost = cost;
            chosenSplitNo = splitNo;
            chosenAxis = axis;
            splitKind = kind;
    //        std::cout << " ***";
        }
    //    std::cout << std::endl;
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
    struct ObjectSlice{
        ObjectSlice() : count(0) {}

        int leftCount() const {
            return count;
        }

        int rightCount() const {
            return count;
        }

        AABB bounds;
        int count;
    };

    // Slice (or bucket) used when trying a Spatial split
    struct SpatialSlice{
        SpatialSlice() : entryCount(0), exitCount(0) {}

        int leftCount() const {
            return entryCount;
        }

        int rightCount() const {
            return exitCount;
        }

        AABB bounds;
        int entryCount, exitCount;
    };


    // calculate cost after slicing
    template<class SliceArrayType>
    static void FindMinCostSplit(
            SliceArrayType const& slices,  // in: slices to consider
            float boundingSurfaceArea,     // in: surface area of extrema bounding box
            int axis,                      // in: axis we're splitting on
            SplitKind kind,                // in: kind of split we're performing (for stats purposes)
            SplitDecision& decision) {     // out: resultant decision

        int totalLeft = 0;
        int totalRight = 0;
        for(auto const& slice : slices) {
            totalLeft += slice.leftCount();
            totalRight += slice.rightCount();

            if(slice.leftCount() > 0 || slice.rightCount() > 0) {
                slice.bounds.sanityCheck();
            }
        }
        //std::cout << " tl " << totalLeft << " tr " << totalRight << std::endl;
        //assert(totalLeft == totalRight);

        //std::cout << std::endl;
        for(unsigned int i = 0; i < (slices.size() - 1) ; i++) {
            // glue slices together into a left slice and a right slice
            AABB leftBounds;
            int leftCount = 0;

            for(unsigned int j = 0; j <= i; j++){
                leftBounds = unionAABB(leftBounds, slices[j].bounds);
                leftCount += slices[j].leftCount();
//                std::cout << "L";
            }

            AABB rightBounds;
            int rightCount = 0;

            for(unsigned int j = i+1; j < slices.size(); j++){
                rightBounds = unionAABB(rightBounds, slices[j].bounds);
                rightCount += slices[j].rightCount();
//                std::cout << "R";
            }

            if(leftCount == 0 && rightCount == 0)
                continue;
            //assert(leftCount > 0);
            //assert(rightCount > 0);

            float areaLeft = 0.0f;
            float areaRight = 0.0f;

            if(leftCount > 0) {
                leftBounds.sanityCheck();
                areaLeft = surfaceAreaAABB(leftBounds);
            }

            if(rightCount > 0) {
                rightBounds.sanityCheck();
                areaRight = surfaceAreaAABB(rightBounds);
            }
            
            float cost = (1 + (leftCount * areaLeft + rightCount * areaRight) / boundingSurfaceArea);

            decision.addCandidate(cost, i, axis, kind);
        }
        //std::cout << std::endl;
    }

    // tries an SAH Object split on the given axis.
    // may be a no-op if the given axis is zero length
    static void TryObjectSplit(
            TrianglePosSet const& triangles,  // in: master triangle array
            TriangleMapping const& indicies,  // in: set of triangle indicies to split 
            float boundingSurfaceArea,        // in: surface area of extrema bounding box
            AABB const& centroidBounds,       // in: bounds of this set of triangles
            int axis,                         // in: axis to test
            SplitDecision& decision) {        // out: resultant decision
        assert(indicies.size() > 1);

        // slice parent bounding box into slices along the longest axis
        // and count the triangle centroids in it
        typedef std::array<ObjectSlice, SLICES_PER_AXIS> SliceArray;
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

        FindMinCostSplit(slices, boundingSurfaceArea, axis, OBJECT, decision);
    }

    // slice (clip) a triangle by an axis-aligned plane perpendicular to @axis at point @splitPoint
    static void AAplaneTriangle(
            TrianglePos const& t, 
            int axis, 
            float splitPoint, 
            VecPair& res) {
        
        assert(splitPoint > t.getMinCoord(axis));
        assert(splitPoint < t.getMaxCoord(axis));

        // grab the 'other 2' axis..
        int a1 = (axis + 1) % 3;
        int a2 = (axis + 2) % 3;
        unsigned int current = 0;

        for(int i = 0; i < 3; i++) {
            const glm::vec3& v0 = t.v[i];
            const glm::vec3& v1 = t.v[(i + 1) % 3];

            // if at different sides of the splitpoint...
            if(((v0[axis] <= splitPoint) && (v1[axis] > splitPoint)) ||
               ((v1[axis] <= splitPoint) && (v0[axis] > splitPoint))){
                // intersect
                float dx = (v1[axis] - splitPoint) / (v1[axis] - v0[axis]);

                assert(current < res.size());
                res[current][axis] = splitPoint;
                res[current][a1] = ((v1[a1] - v0[a1]) * dx) + v0[a1];
                res[current][a2] = ((v1[a2] - v0[a2]) * dx) + v0[a2];
                current++;
            }
        }
        // should have intersected exactly twice
        assert(current == 2);
    }

    // tries an SAH Spatial split on the given axis. 
    // may be a no-op if the given axis is zero length
    static void TrySpatialSplit(
            TrianglePosSet const& triangles,  // in: master triangle array
            TriangleMapping const& indicies,  // in: set of triangle indicies to split 
            float boundingSurfaceArea,        // in: surface area of extrema bounding box
            AABB const& extremaBounds,        // in: bounds of this set of triangles
            int axis,                         // in: axis to test
            SplitDecision& decision) {        // out: resultant decision

        assert(indicies.size() > 1);

        // slice parent bounding box into slices along the longest axis
        // and count the triangle centroids in it
        typedef std::array<SpatialSlice, SLICES_PER_AXIS> SliceArray;
        SliceArray slices;

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

                // tri clips low side of slice?
                bool clippedLow = false;

                if(tri.getMinCoord(axis) < sliceLow) {
                    VecPair intersect;
                    AAplaneTriangle(tri, axis, sliceLow, intersect);
                    slice.bounds = unionVecPair(slice.bounds, intersect);
                    clippedLow = true;
                } else {
                    // doesn't clip low side - tri must start in this slice
                    slice.entryCount++;
                }

                // tri clips high side of slice?
                bool clippedHigh = false;

                if(tri.getMaxCoord(axis) > sliceHigh) {
                    VecPair intersect;
                    AAplaneTriangle(tri, axis, sliceHigh, intersect);
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
            
        FindMinCostSplit(slices, boundingSurfaceArea, axis, SPATIAL, decision);
    }

    static bool GetSplit(
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

        // walk the 3 axis, find the lowest cost split
        // ... firstly, try object splits
        for(int axis = 0; axis < 3; axis++) { 
            TryObjectSplit(triangles, indicies, boundingArea, centroidBounds, axis, decision);
        }

        decision.sanityCheck();

        for(int axis = 0; axis < 3; axis++) {
            TrySpatialSplit(triangles, indicies, boundingArea, extremaBounds, axis, decision);
        }

        std::cout << decision;
        decision.sanityCheck();

//        std::cout << " chosenAxis " << chosenAxis;
//        std::cout << " chosenSplitNo " << chosenSplitNo;

        // check termination heurisic...
        if(decision.minCost > indicies.size()) {
            return false; // no splitting here, chopper. make a leaf with this triangle set
        }

        if(decision.splitKind == OBJECT) {
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
        } else {
            const float low = extremaBounds.low[decision.chosenAxis];
            const float high = extremaBounds.high[decision.chosenAxis];
        
            // at this point, low must be < high (ie not equal) as we've chosen it as a split axis
            // this means there must be a point in this axis we can split the triangles
            assert(low < high);
            const float range = high - low;
            const float sliceWidth = range / (float)SLICES_PER_AXIS;

//            std::cout << std::endl;
            std::cout << " low " << low;
            std::cout << " high " << high;
            std::cout << " sliceWidth " << sliceWidth;

            float splitPoint = ((decision.chosenSplitNo + 1) * sliceWidth) + low;

            std::cout << " splitPoint " << splitPoint;

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

            std::cout << " count " << indicies.size();
            std::cout << " leftCount " << leftIndicies.size();
            std::cout << " rightCount " << rightIndicies.size();

            assert(leftIndicies.size() + rightIndicies.size() >= indicies.size());
            assert(leftIndicies.size() <= indicies.size());
            assert(rightIndicies.size() <= indicies.size());

            // so this sucks. If all the triangles land on one side, then don't split; this 
            // would mean we'll recurse forever.
            if(leftIndicies.size() == indicies.size() || rightIndicies.size() == indicies.size()) {
                std::cout << " -- sigh, nasty term"  << std::endl;
                return false;
            }
        }

        std::cout << std::endl;
        return true; // yes, we split!
    }
};

BVH* buildSBVH(Scene& s){
    std::cout << "building SBVH" << std::endl;
    BVH* bvh = buildBVH<SBVHSplitter>(s);
    return bvh;
}

