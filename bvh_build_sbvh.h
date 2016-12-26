#pragma once

// build an SBVH - that is a Split Bounding Volume Hierachy
struct SBVHSplitter {
    // max number of slices (buckets) to test when splitting
    static constexpr int SLICES_PER_AXIS = 8;

    // Slice (or bucket) used when trying a split
    struct Slice{
        Slice() : count(0) {}

        AABB bounds;
        int count;
    };

    typedef std::array<Slice, SLICES_PER_AXIS> SliceArray;

    struct SplitDecision {
        SplitDecision(): minCost(INFINITY), chosenSplitNo(-1), chosenAxis(-1) {}

        // once the dust has settled, check we've got a result
        void sanityCheck() {
            assert(minCost < INFINITY);
            assert(chosenSplitNo >= 0);
            assert(chosenSplitNo < SLICES_PER_AXIS);
            assert(chosenAxis >= 0);
            assert(chosenAxis < 3);
        }

        void addCandidate(float cost, int splitNo, int axis) {
            if(cost < minCost) {
                minCost = cost;
                chosenSplitNo = splitNo;
                chosenAxis = axis;
            }
        }

        float minCost;
        int chosenSplitNo;
        int chosenAxis;
    };

    // calculate cost after slicing
    static void FindMinCostSplit(
            SliceArray const& slices,  // in: slices to consider
            float boundingSurfaceArea, // in: surface area of extrema bounding box
            int axis,                  // in: axis we're splitting on
            SplitDecision& decision) { // out: resultant decision

        for(unsigned int i = 0; i < (slices.size() - 1) ; i++) {
            // glue slices together into a left slice and a right slice
            Slice left; 
            for(unsigned int j = 0; j <= i; j++){
                left.bounds = unionAABB(left.bounds, slices[j].bounds);
                left.count += slices[j].count;
            }

            Slice right;
            for(unsigned int j = i+1; j < (slices.size() - 1); j++){
                right.bounds= unionAABB(right.bounds, slices[j].bounds);
                right.count += slices[j].count;
            }

            //assert(left.count > 0);
            //assert(right.count > 0);
            float al = surfaceAreaAABB(left.bounds);
            float ar = surfaceAreaAABB(right.bounds);
            
            float cost = (1 + (left.count * al + right.count * ar) / boundingSurfaceArea);

            decision.addCandidate(cost, i, axis);
        }
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

            assert(sliceNo < slices.size());
            slices[sliceNo].bounds = unionAABB(slices[sliceNo].bounds, triBounds);
            slices[sliceNo].count++;
        }

        FindMinCostSplit(slices, boundingSurfaceArea, axis, decision);
    }

    // slice (clip) a triangle by an axis-aligned plane perpendicular to @axis at point @splitPoint
    static void AAplaneTriangle(
            TrianglePos const& t, 
            int axis, 
            float splitPoint, 
            std::array<glm::vec3, 2>& res) {

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
        SliceArray slices;

        const float low = extremaBounds.low[axis];
        const float high = extremaBounds.high[axis];
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
                float sliceLow = (sliceNo * sliceWidth) + low;
                float sliceHigh = sliceLow + sliceWidth;
                assert(sliceHigh <= high);

                // does this tri fall in this slice?
                if(tri.getMinCoord(axis) > sliceHigh || tri.getMaxCoord(axis) < sliceLow)
                    continue;
            
                if(tri.getMinCoord(axis) < sliceLow) {
                    std::array<glm::vec3, 2> intersect;
                    AAplaneTriangle(tri, axis, sliceLow, intersect);
                    slices[sliceNo].bounds = unionPoint(slices[sliceNo].bounds, intersect[0]);
                    slices[sliceNo].bounds = unionPoint(slices[sliceNo].bounds, intersect[1]);
                    slices[sliceNo].count++;
                }

                if(tri.getMaxCoord(axis) > sliceHigh) {
                    std::array<glm::vec3, 2> intersect;
                    AAplaneTriangle(tri, axis, sliceHigh, intersect);
                    slices[sliceNo].bounds = unionPoint(slices[sliceNo].bounds, intersect[0]);
                    slices[sliceNo].bounds = unionPoint(slices[sliceNo].bounds, intersect[1]);
                    slices[sliceNo].count++;
                }
            }
        }
            
#if 0


            // drop this centroid into a slice
            const float pos = tri.getAverageCoord(axis);
            const float ratio = ((pos - low) / sliceWidth);
            unsigned int sliceNo = ratio * SLICES_PER_AXIS;

            if(sliceNo == SLICES_PER_AXIS)
                sliceNo--;

            const AABB triBounds = triangleBounds(tri);

            assert(sliceNo < slices.size());
            slices[sliceNo].aabb = unionAABB(slices[sliceNo].aabb, triBounds);
            slices[sliceNo].count++;
#endif

        FindMinCostSplit(slices, boundingSurfaceArea, axis, decision);
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
        // triangle centroids
        const AABB centroidBounds = buildAABBCentroid(triangles, indicies, 0, indicies.size());

        // bounds of centroids must be within total triangle bounds
        centroidBounds.sanityCheck();
        assert(containsAABB(extremaBounds, centroidBounds));

        SplitDecision decision;

        // walk the 3 axis, find the lowest cost split
        // ... firstly, try object splits
        for(int axis = 0; axis < 3; axis++) {
//TryObjectSplit(triangles, indicies, boundingArea, centroidBounds, axis, decision);
        }

//        decision.sanityCheck();

        for(int axis = 0; axis < 3; axis++) {
            TrySpatialSplit(triangles, indicies, boundingArea, extremaBounds, axis, decision);
        }

        decision.sanityCheck();

//        std::cout << " chosenAxis " << chosenAxis;
//        std::cout << " chosenSplitNo " << chosenSplitNo;

        // check termination heurisic...
        if(decision.minCost > indicies.size()) {
            return false; // no splitting here, chopper. make a leaf with this triangle set
        }

        const float low = centroidBounds.low[decision.chosenAxis];
        const float high = centroidBounds.high[decision.chosenAxis];
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
        return true; // yes, we split!
    }
};

BVH* buildSBVH(Scene& s){
    std::cout << "building SBVH" << std::endl;
    BVH* bvh = buildBVH<SBVHSplitter>(s);
    return bvh;
}

#if 0   
        // MIN/MAX BASED
class BoundsPartitioner {
        for(unsigned int i = 0; i < fromIndicies.size(); i++) {
            unsigned int idx = fromIndicies[i];
            float valMin = triangles[idx].getMinCoord(splitAxis);
            float valMax = triangles[idx].getMaxCoord(splitAxis);
            
//            std::cout << "idx " << idx;
//            std::cout << " valMin " << valMin << " valMax " << valMax;
//            std::cout << " leftMax " << leftMax << " rightMin " << rightMin << std::endl;;
            // FIXME: beware of >= or <= cases (hairy with floats)
            // .. we could miss triangles here
            // We want to include the triangle if at least one part of it is within the range
            if(valMin <= leftMax)
                leftIndicies.push_back(idx);
            if(valMax >= rightMin)
                rightIndicies.push_back(idx);
        }
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

        assert(indicies.size()>0);
        // size of the aabb
        glm::vec3 diff = bounds.high - bounds.low;

        // try a few places to split and find the one resulting in the smallest area
        float smallest_area_so_far = INFINITY;
        bool split_good_enough = false;
        printf("splitting bounding box x:%f<%f, y:%f<%f, z:%f<%f\n",
                bounds.low.x, bounds.high.x,
                bounds.low.y, bounds.high.y,
                bounds.low.z, bounds.high.z);
                     
        for(int axis=0; axis<3; axis++) for(int split=1; split<8; split++){
            float trySplitPoint = bounds.low[axis] + ((float)split*(diff[axis]/8.f));
            printf("  trying split %c=%f ","xyz"[axis],trySplitPoint);
            // we keep a running total of the size of the left and right bounding box
            AABB left, right;
            int triangles_in_left = 0;
            int triangles_in_right = 0;
            for(int t:indicies){ // for each triangle
                TrianglePos const& triangle = triangles[t];
                // find out if the triangle belongs to left, right or both
                int vertices_in_left = 0;
                int vertices_in_right= 0;
                for(int vertex=0; vertex<3; vertex++){
                    if(triangle.v[vertex][axis] <= trySplitPoint) vertices_in_left++;
                    if(triangle.v[vertex][axis] >= trySplitPoint) vertices_in_right++;
                }
                // if the triangle is completely at the left side of the
                // splitting plane, grow the left bounding box to include the
                // triangle
                if(vertices_in_left==3){
                    triangles_in_left++;
                    for(int v=0;v<3;v++){
                        left.low = glm::min(left.low, triangle.v[v]);
                        left.high= glm::max(left.high,triangle.v[v]);
                    }
                }
                // same for the right side, not mutually exclusive with the 
                // statement above because of the edge case where the
                // triangle lies exactly on the split plane
                if(vertices_in_right==3){
                    triangles_in_right++;
                    for(int v=0;v<3;v++){
                        right.low = glm::min(right.low, triangle.v[v]);
                        right.high= glm::max(right.high,triangle.v[v]);
                    }
                }
                // if the triangle intersects the split plane, use the bounding
                // box of the cut triangle to grow both sides
                if(vertices_in_left!=3 && vertices_in_right!=3){
                    triangles_in_right++;
                    triangles_in_left++;
                    #if 0
                    // find intersection points
                    glm::vec3 t0,t1;
                    AAplaneTriangle(triangle, axis, trySplitPoint, &t0, &t1);
                    // grow the bounding boxes including these points
                    glm::vec3 big[4], small[3]; 
                    if(vertices_in_left>vertices_in_right){
                    }else{

                    }
                    #else
                    // suboptimal but easy
                    for(int v=0;v<3;v++){
                        left.low   = glm::min(left.low, triangle.v[v]);
                        left.high  = glm::max(left.high,triangle.v[v]);
                        right.low  = glm::min(right.low, triangle.v[v]);
                        right.high = glm::max(right.high,triangle.v[v]);
                    }
                    left.high[axis] = trySplitPoint;
                    right.low[axis] = trySplitPoint;
                    #endif
                }
            }
            // now check if the split is the best so far
            float al = surfaceAreaAABB(left);
            float ar = surfaceAreaAABB(right);
            float area= al+ar;
            int triangle_count = indicies.size();
            printf("%d -> %d/%d\n",triangle_count, triangles_in_left, triangles_in_right);
            if(area<smallest_area_so_far
            && triangles_in_left != triangle_count
            && triangles_in_right!= triangle_count){
                // if it is the best so far, check if we should split at all
                printf("  testing: al%f*tl%d+ar%f*tr%d < a%f*t%d\n",al,triangles_in_left,ar,triangles_in_right,area,triangle_count);
                float should_share = al*(float)triangles_in_left + ar*(float)triangles_in_right;
                float should_stop  = area*(float)triangle_count;
                if(should_share<should_stop){
                    printf("    best so far: area:%f triangles_L:%d triangles_R:%d total:%d\n", area, triangles_in_left, triangles_in_right, triangle_count);
                    printf("      left:%f<%f right:%f<%f\n", left.low[axis], left.high[axis], right.low[axis], right.high[axis]);
                    // we should split, set all the output variables
                    assert(triangle_count > 1);
                    assert(triangles_in_left >= 1);
                    assert(triangles_in_right >= 1);
                    assert(triangles_in_left  != triangle_count);
                    assert(triangles_in_right != triangle_count);
                    smallest_area_so_far = area;
                    split_good_enough = true;
                    rightMin = fmax(right.low[axis], left.low[axis]);
                    leftMax  = fmin(right.high[axis], left.high[axis]);
                    splitAxis= axis;
                }
            }
        }

        printf("  RESULT: %s\n", split_good_enough?"split good enough":"not worth splitting");
        return split_good_enough;
    }
};

#endif
