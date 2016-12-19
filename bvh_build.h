#pragma once

#include "bvh.h"
#include "bvh_diag.h"
#include "scene.h"

enum BVHMethod {
    BVHMethod_STUPID,
    BVHMethod_CENTROID_SAH,
//    BVHMethod_SAH,
    __BVHMethod_MAX
};

const char* BVHMethodStr(BVHMethod m) {
    switch(m) {
        case BVHMethod_STUPID: return "STUPID";
        case BVHMethod_CENTROID_SAH: return "Centroid SAH";
//        case BVHMethod_SAH:    return "SAH";
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
#if 0   
        // MIN/MAX BASED
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
#else
        // CENTROID BASED
        for(unsigned int idx:fromIndicies){
            if(triangles[idx].getCentroid()[splitAxis] <= leftMax)
                leftIndicies.push_back(idx);
            else
                rightIndicies.push_back(idx);
        }
#endif

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
        assert(leftIndicies.size()  < fromIndicies.size());
        assert(rightIndicies.size() < fromIndicies.size());

        // recurse
        subdivide<Splitter>(triangles, bvh, left, leftIndicies, splitAxis);
        subdivide<Splitter>(triangles, bvh, right, rightIndicies, splitAxis);
    }
    // now we're done, node should be fully setup. check
    assert(surfaceAreaAABB(node.bounds) > 0.0f);
    assert(node.leftFirst >= 0);
    if(node.isLeaf()){
        assert((node.leftFirst + node.count) <= bvh.indicies.size());
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

    // For the stupid splitter, we should have a single node, and it should be a leaf
    assert(bvh->nodeCount() == 1);
    assert(bvh->root().isLeaf());
    return bvh;
}

struct Slice{
    Slice(){
        count = 0;
    }
    AABB aabb;
    int  count;
};

struct CentroidSAHSplitter {
    static bool GetSplit(
            TrianglePosSet const& triangles,// in: master triangle array
            TriangleMapping const& indicies,// in: set of triangle indicies to split 
            AABB bounds,                    // in: bounds of this set of triangles
            unsigned int lastAxis,          // in: the axis on which the parent was split
            unsigned int& axis,             // out: axis on which to split
            float& leftMax,                 // out: max point to include in left set 
            float& rightMin) {              // out: min point to include in right set
        if(indicies.size()<=3) return false;

        bounds = AABB();
        for(int const i:indicies){
            glm::vec3 const c = triangles[i].getCentroid();
            bounds = unionPoint(bounds,c);
        }

        // find longest axis
        glm::vec3 diff = bounds.high - bounds.low;
        axis = diff[0] > diff[1] ?  
               diff[0] > diff[2] ? 0 : 2
             : diff[1] > diff[2] ? 1 : 2;
        
        static long long callnr=0;
        printf("%-4lldsplitting bounding box x:%f<%f, y:%f<%f, z:%f<%f across %c axis (%lu triangles)\n",
                callnr++,
                bounds.low.x, bounds.high.x,
                bounds.low.y, bounds.high.y,
                bounds.low.z, bounds.high.z,
                "xyz"[axis],indicies.size());

        // slice parent bounding box into slices along the longest axis
        // and count the triangle centroids in it
        constexpr int MAX_SLICES = 8;
        Slice slices[MAX_SLICES];
        for(int const idx:indicies){
            glm::vec3 const c = triangles[idx].getCentroid();
            int s = MAX_SLICES*((c[axis] - bounds.low[axis])/(bounds.high[axis]-bounds.low[axis]));
            if(s>=MAX_SLICES) s=MAX_SLICES-1;
            AABB triangle;
            for(int i=0;i<3;i++) triangle = unionPoint(triangle,triangles[idx].v[i]);
            slices[s].aabb = unionAABB(slices[s].aabb, triangle);
            slices[s].count++;
        }

        // print out our slices
        for(int i=0; i<MAX_SLICES; i++){
            float const sliceWidth = fabs(bounds.high[axis]-bounds.low[axis])/(float)MAX_SLICES;
            float const sliceMin = bounds.low[axis]+i*sliceWidth;
            float const sliceMax = bounds.low[axis]+(i+1)*sliceWidth;
            printf("    slice %d: t:%d, x:%f<%f, y:%f<%f, z:%f<%f, w:%f s:%f<%f\n",
                i, slices[i].count,
                slices[i].aabb.low.x, slices[i].aabb.high.x,
                slices[i].aabb.low.y, slices[i].aabb.high.y,
                slices[i].aabb.low.z, slices[i].aabb.high.z,
                sliceWidth, sliceMin, sliceMax);
        }

        // calculate cost after each slice
        float boundingArea = surfaceAreaAABB(bounds);
        float cost[MAX_SLICES-1];
        for(int i=0; i<MAX_SLICES-1; i++){
            printf("    ");
            // glue slices together into a left slice and a right slice
            Slice left; 
            for(int j=0; j<=i; j++){
                printf("L");
                left.aabb = unionAABB(left.aabb, slices[j].aabb);
                left.count += slices[j].count;
            }
            Slice right;
            for(int j=i+1; j<MAX_SLICES; j++){
                printf("R");
                right.aabb = unionAABB(right.aabb, slices[j].aabb);
                right.count += slices[j].count;
            }
            float al = surfaceAreaAABB(left.aabb);
            float ar = surfaceAreaAABB(right.aabb);
            cost[i] = 1 + (left.count  * al + right.count * ar) / boundingArea;
            printf(" %d|%d: cost:%f, count:%d/%d, area:%f/%f x:%f<%f, y:%f<%f, z:%f<%f\n",
                    i, i+1, cost[i], left.count, right.count,
                    al, ar,
                    left.aabb.low.x, left.aabb.high.x,
                    left.aabb.low.y, left.aabb.high.y,
                    left.aabb.low.z, left.aabb.high.z);
        }

        // find minimal permutation
        int   minIdx = 0;
        float minCost=cost[0];
        for(int i=1; i<MAX_SLICES-1; i++){
            if(cost[i]<cost[minIdx]){
                minIdx = i;
                minCost = cost[i];
            }
        }

        leftMax  = slices[minIdx].aabb.high[axis];
        int second_index=0;
        for(int i=minIdx+1; i<MAX_SLICES; i++){
            if(slices[i].count>0){
                rightMin = slices[i].aabb.low[axis];
                second_index=i;
                break;
            }
        }

        bool split_good_enough = cost[minIdx] < indicies.size();
        printf("    %s: %d|%d, cost:%f, lmax:%f, rmin:%f\n", 
                split_good_enough?"winner":"NOT SPLITTING",
                minIdx, second_index, 
                cost[minIdx], leftMax, rightMin);
        
        static AABB last_aabb;
        if(bounds==last_aabb){
            printf("    >>> SAME BOUNDS AS LAST CALL! <<<\n");
            exit(1);
        }
        last_aabb = bounds;
        //if(callnr>10) exit(0);
        return split_good_enough;
    }
};

inline BVH* buildCentroidSAHBVH(Scene& s) {
    std::cout << "building centroid SAH BVH" << std::endl;
    BVH* bvh = buildBVH<CentroidSAHSplitter>(s);
    return bvh;
}

#if 0
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
#endif

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
                TrianglePosition const& triangle = triangles[t];
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

BVH* buildSAHBVH(Scene& s){
    std::cout << "building SAH BVH" << std::endl;
    BVH* bvh = buildBVH<SAHSplitter>(s);
    return bvh;
}

inline BVH* buildBVH(Scene& s, BVHMethod method) {
    BVH* bvh = nullptr;

    switch(method) {
        case BVHMethod_STUPID: bvh = buildStupidBVH(s); break;
        case BVHMethod_CENTROID_SAH: bvh=buildCentroidSAHBVH(s); break;
        //case BVHMethod_SAH:    bvh = buildSAHBVH(s);    break;
        case __BVHMethod_MAX: assert(false); break; // shouldn't happen
    };

    std::cout << "bvh node count " << bvh->nextFree << std::endl;
    std::cout << "triangle count " << s.primitives.triangles.size() << std::endl;
    std::cout << "root AABB " << bvh->root().bounds << std::endl;

    sanityCheckBVH(*bvh, s.primitives.pos);

    return bvh;
}
