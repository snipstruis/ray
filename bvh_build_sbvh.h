#pragma once

// build an SBVH - that is a Split Bounding Volume Hierachy
struct SBVHSplitter {
    // max number of slices (buckets) to test when splitting
    static constexpr int SLICES_PER_AXIS = 8;

    // Slice (or bucket) used when trying a split
    struct Slice{
        Slice() : count(0) {}

        AABB aabb;
        int count;
    };

    typedef std::array<Slice, SLICES_PER_AXIS> SliceArray;

    // calculate cost after slicing
    static void FindMinCostSplit(
            SliceArray const& slices,   // in: slices to consider
            float boundingSurfaceArea, // in: surface area of extrema bounding box
            float& minCost,            // out: min cost split we found
            int& splitNo) {            // out: split point for this min cost

        std::array<float, SLICES_PER_AXIS - 1> costs;

        for(unsigned int i = 0; i < costs.size() ; i++) {
            // glue slices together into a left slice and a right slice
            Slice left; 
            for(unsigned int j = 0; j <= i; j++){
                left.aabb = unionAABB(left.aabb, slices[j].aabb);
                left.count += slices[j].count;
            }

            Slice right;
            for(unsigned int j = i+1; j < costs.size(); j++){
                right.aabb = unionAABB(right.aabb, slices[j].aabb);
                right.count += slices[j].count;
            }

            //assert(left.count > 0);
            //assert(right.count > 0);
            float al = surfaceAreaAABB(left.aabb);
            float ar = surfaceAreaAABB(right.aabb);
            
            float cost = (1 + (left.count * al + right.count * ar) / boundingSurfaceArea);
            costs[i] = cost;
        }

        // return minimal permutation
        splitNo = 0;
        minCost = costs[0];
        
        for(unsigned int i = 1; i < costs.size(); i++){
            if(costs[i] < minCost) {
                splitNo = i;
                minCost = costs[i];
            }
        }
    }

    // tries an SAH Object split on the given axis. Returns false if no split could be done.
    // note this only returns the split bucket and the cost - it doesn't actually change anything
    static bool TryObjectSplit(
            TrianglePosSet const& triangles,  // in: master triangle array
            TriangleMapping const& indicies,  // in: set of triangle indicies to split 
            float boundingSurfaceArea,        // in: surface area of extrema bounding box
            AABB const& centroidBounds,       // in: bounds of this set of triangles
            int axis,                         // in: axis to test
            float& minCost,                   // out: min cost split we found
            int& splitNo) {                   // out: split point for this min cost
        assert(indicies.size() > 1);

        // slice parent bounding box into slices along the longest axis
        // and count the triangle centroids in it
        SliceArray slices;

        const float low = centroidBounds.low[axis];
        const float high = centroidBounds.high[axis];
        // it's possible to get a case where low==high. just ignore this axis
        if(!(low < high))
            return false;

        const float sliceWidth = high - low;

        for(int const idx : indicies) {
            const TrianglePos& tri = triangles[idx];
            
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
        }

        FindMinCostSplit(slices, boundingSurfaceArea, minCost, splitNo);

        return true; // success!
    }

    // tries an SAH Spatial split on the given axis. Returns false if no split could be done.
    // similarly to TryObjectSplit, this is 'read only'
    static bool TrySpatialSplit(
            TrianglePosSet const& triangles,  // in: master triangle array
            TriangleMapping const& indicies,  // in: set of triangle indicies to split 
            float boundingSurfaceArea,        // in: surface area of extrema bounding box
            AABB const& extremaBounds,        // in: bounds of this set of triangles
            int axis,                         // in: axis to test
            float& minCost,                   // out: min cost split we found
            int& splitNo) {                   // out: split point for this min cost

        assert(indicies.size() > 1);

        return false;

        // slice parent bounding box into slices along the longest axis
        // and count the triangle centroids in it
        SliceArray slices;

        const float low = extremaBounds.low[axis];
        const float high = extremaBounds.high[axis];
        // it's possible to get a case where low==high. just ignore this axis
        if(!(low < high))
            return false;

        const float sliceWidth = high - low;

        for(int const idx : indicies) {
            const TrianglePos& tri = triangles[idx];
            
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
        }

        FindMinCostSplit(slices, boundingSurfaceArea, minCost, splitNo);

        return true; // success!
    }

    static bool GetSplit(
            TrianglePosSet const& triangles,  // in: master triangle array
            TriangleMapping const& indicies,  // in: set of triangle indicies to split 
            AABB const& bounds,               // in: bounds of this set of triangles
            TriangleMapping& leftIndicies,    // out: resultant left set
            TriangleMapping& rightIndicies) { // out: resultant right set

        bounds.sanityCheck();

        // force a leaf if we're only given 3 triangles.
        if(indicies.size() <= 3) 
            return false;

        const float boundingArea = surfaceAreaAABB(bounds);

        // the bounds that we're given is an extrema brounds. We also need one that surrounds the 
        // triangle centroids
        const AABB centroidBounds = buildAABBCentroid(triangles, indicies, 0, indicies.size());

        // bounds of centroids must be within total triangle bounds
        centroidBounds.sanityCheck();
        assert(containsAABB(bounds, centroidBounds));

        float minCost = INFINITY;
        unsigned int chosenSplitNo;
        int chosenAxis = -1;

        // walk the 3 axis, find the lowest cost split
        // ... firstly, try object splits
        for(int axis = 0; axis < 3; axis++) {
            float cost;
            int splitNo;

            if(!TryObjectSplit(triangles, indicies, boundingArea, centroidBounds, axis, cost, splitNo))
                continue;

            if(cost < minCost) {
                minCost = cost;
                chosenSplitNo = splitNo;
                chosenAxis = axis;
            }
        }

        assert(minCost < INFINITY);
        assert(chosenAxis >= 0);
        assert(chosenAxis < 3);

        for(int axis = 0; axis < 3; axis++) {
            float cost;
            int splitNo;

            if(!TrySpatialSplit(triangles, indicies, boundingArea, extremaBounds, axis, cost, splitNo))
                continue;

            if(cost < minCost) {
                minCost = cost;
                chosenSplitNo = splitNo;
                chosenAxis = axis;
            }
        }

        assert(minCost < INFINITY);
        assert(chosenAxis >= 0);
        assert(chosenAxis < 3);

//        std::cout << " chosenAxis " << chosenAxis;
//        std::cout << " chosenSplitNo " << chosenSplitNo;

        // check termination heurisic...
        if(minCost > indicies.size()) {
            return false; // no splitting here, chopper. make a leaf with this triangle set
        }

        const float low = centroidBounds.low[chosenAxis];
        const float high = centroidBounds.high[chosenAxis];
        assert(low < high);
        const float sliceWidth = high - low;

        // ok, we're going to split. parition the indicies based on bucket
        for(unsigned int idx : indicies) {
            // determine slice in which this one belongs
            const float val = triangles[idx].getAverageCoord(chosenAxis);
            const float ratio = ((val - low) / sliceWidth);
            unsigned int sliceNo = ratio * SLICES_PER_AXIS;
            if(sliceNo == SLICES_PER_AXIS)
                sliceNo--;

            if(sliceNo <= chosenSplitNo)
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

void AAplaneTriangle(TrianglePos const& t, int axis, float plane, glm::vec3* p0, flm::vec3* p1){
    int points = 0;
    glm::vec3 p[2];
    int a1=(axis+1)%3;
    int a2=(axis+2)%3;
    for(int i=0;i<3;i++){
        // if at different sides of the splitpoint...
        glm::vec3 t0 = t.v[i];
        glm::vec3 t1 = t.v[(i+1)%3];
        if((t0[axis] <= plane) ^^ (t1[axis] <= plane)){
            // intersect
            float dx = (t0[axis]-plane)/(t0[axis]-t1[axis]);
            p[points][axis] = plane;
            p[points][a1] = dx*t0;
            p[points][a2] = dx*t1;
            points++;
        }
    }
    *p0=p[0];
    *p1=p[1];
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
