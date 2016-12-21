#pragma once

#include "aabb.h"
#include "bvh.h"
#include "primitive.h"
#include "debug_print.h"

#include "glm/vec3.hpp"

// minimal intersection result
// note: if dist == INFINITY, triangle is undefined.
struct MiniIntersection {
    MiniIntersection(float _distance, int _triangle) : distance(_distance), triangle(_triangle) {}
    MiniIntersection() : distance(INFINITY) {} 
    float distance;         // dist to intersection 
    unsigned int triangle;  // triangle number
    int nodeIndex;
    int leafDepth=0;
};

// used to visualise which node/bounds we intersected with
struct DiagnosticCollector {
    DiagnosticCollector() : splitsTraversed(0), trianglesChecked(0), leafsChecked(0) {}

    void incSplitsTraversed() {
        splitsTraversed++;
    }

    void incTrianglesChecked() {
        trianglesChecked++;
    }

    void incLeafsChecked() {
        leafsChecked++;
    }

    int splitsTraversed;
    int trianglesChecked;
    int leafsChecked;
};

// used when we don't care for stats
struct NullCollector {
    static void incSplitsTraversed() {}
    static void incTrianglesChecked() {}
    static void incLeafsChecked() {}
};

// Find the closest intersection with any primitive
enum class IntersectMode{
    CLOSEST,
    ANY,
    DIAG
};

enum class TraversalMode{
    UNORDERED,
    CENTROID,
    MAX
} traversalMode = TraversalMode::UNORDERED;

char const * const traversalstr[] = {
    "unordered",
    "ordered(centroid)",
    "SHOULD NOT HAPPEN"
};

template<IntersectMode MODE, TraversalMode TRAV, class DiagnosticCollectorType>
MiniIntersection traverseBVH(
        BVH const& bvh, 
        int nodeIndex, 
        Primitives const& prims, 
        Ray const& ray,
        glm::vec3 const& rayInvDir,
        float const maxDist,
        DiagnosticCollectorType& diag) {
    auto const& node = bvh.getNode(nodeIndex);

    // does this ray miss us all together?
    if(rayIntersectsAABB(node.bounds, ray.origin, rayInvDir) == INFINITY){
        return MiniIntersection();
    }

    if(!node.isLeaf()) {
        if constexpr(MODE==IntersectMode::DIAG) diag.incSplitsTraversed();
        // we are not at a leaf yet - consider both children

        // ordered traveral
        if constexpr(TRAV==TraversalMode::CENTROID){
            glm::vec3 leftCentroid  = centroidAABB(bvh.getNode(node.leftIndex()).bounds);
            glm::vec3 rightCentroid = centroidAABB(bvh.getNode(node.rightIndex()).bounds);
            // find the biggest axis
            float x = fabs(leftCentroid.x-rightCentroid.x);
            float y = fabs(leftCentroid.y-rightCentroid.y);
            float z = fabs(leftCentroid.z-rightCentroid.z);
            int axis = x>y?  x>z?0:2 : y>z?1:2;

            bool left_closer = ray.direction[axis]>0.f;
            int first_index  = left_closer?node.leftIndex():node.rightIndex();
            int second_index = left_closer?node.rightIndex():node.leftIndex();

            float max_distance = rayIntersectsAABB(bvh.getNode(second_index).bounds, ray.origin, rayInvDir);
            auto first = traverseBVH<MODE,TRAV>(bvh, first_index, prims, ray, rayInvDir, maxDist, diag);
            if(max_distance == INFINITY) 
                return first;

            if(first.distance < max_distance){
                auto second = traverseBVH<MODE,TRAV>(bvh, second_index,  prims, ray, rayInvDir, maxDist, diag);
                return second;
            }else return first;
        }else{ // unordered traversal
            auto hitLeft = traverseBVH<MODE,TRAV>(bvh, node.leftIndex(),  prims, ray, rayInvDir, maxDist, diag);
            auto hitRight = traverseBVH<MODE,TRAV>(bvh, node.rightIndex(), prims, ray, rayInvDir, maxDist, diag);

            if(hitLeft.distance == INFINITY && hitRight.distance == INFINITY){
                if(MODE==IntersectMode::DIAG) hitLeft.leafDepth++;
                return MiniIntersection();
            }

            if(hitLeft.distance < hitRight.distance){
                if(MODE==IntersectMode::DIAG) hitLeft.leafDepth++;
                return hitLeft;
            } else {
                if(MODE==IntersectMode::DIAG) hitRight.leafDepth++;
                return hitRight;
            }
        }
    } else {    
        // at a leaf - walk triangles and test for a hit.
        if constexpr(MODE==IntersectMode::DIAG) diag.incLeafsChecked();
        MiniIntersection hit;
        for(unsigned int i = node.first(); i < (node.first() + node.count); i++) {
            if constexpr(MODE==IntersectMode::DIAG) diag.incTrianglesChecked();
            assert(i < bvh.indicies.size());
            unsigned int triangleIndex = bvh.indicies[i];
            TrianglePosition const& t = prims.pos[triangleIndex];
            float distance = moller_trumbore(t, ray);

            if constexpr(MODE==IntersectMode::ANY){ // if checking for any intersection whatsoever
                if(distance > 0 && distance < maxDist){
                    hit.distance = 0;
                    return hit;
                }
            }else{ // if we want to find the closest intersection
                if(distance > 0 && distance < hit.distance) {
                    if constexpr(MODE==IntersectMode::DIAG) hit.nodeIndex = nodeIndex;
                    hit.distance = distance;
                    hit.triangle = triangleIndex;
                }
            }
        }
        return hit;
    }
}

// find closest triangle intersection for ray
template<IntersectMode MODE, TraversalMode TRAV, class DiagnosticCollectorType>
MiniIntersection traverseBVH(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray,
        float const maxDist,
        DiagnosticCollectorType& diag) {

    // calculate 1/direction here once, as it's used repeatedly throughout the recursive chain
    glm::vec3 invDirection(1.0f/ray.direction[0], 1.0f/ray.direction[1], 1.0f/ray.direction[2]);

    return traverseBVH<MODE,TRAV>(bvh, 0, primitives, ray, invDirection, maxDist, diag);
}

#if 0 
MiniIntersection findClosestIntersectionBVH(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray){
    if(traversalMode==TraversalMode::UNORDERED)
        return traverseBVH<IntersectMode::CLOSEST,TraversalMode::UNORDERED>(bvh, primitives, ray, 0.f, nullptr);
    else 
        return traverseBVH<IntersectMode::CLOSEST,TraversalMode::CENTROID>(bvh, primitives, ray, 0.f, nullptr);
}
#endif

template<class DiagnosticCollectorType>
MiniIntersection findClosestIntersectionBVH(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray,
        DiagnosticCollectorType& diag) {

    return (traversalMode==TraversalMode::UNORDERED) ?
        traverseBVH<IntersectMode::DIAG,TraversalMode::UNORDERED>(bvh, primitives, ray, 0.f, diag) :
        traverseBVH<IntersectMode::DIAG,TraversalMode::CENTROID>(bvh, primitives, ray, 0.f, diag);
}

MiniIntersection findClosestIntersectionBVH(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray) {

    NullCollector diag;
    return findClosestIntersectionBVH(bvh, primitives, ray, diag); 
}

template<class DiagnosticCollectorType>
bool findAnyIntersectionBVH(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray,
        float max_length,
        DiagnosticCollectorType& diag) {
    
    MiniIntersection hit = (traversalMode==TraversalMode::UNORDERED) ?
        traverseBVH<IntersectMode::ANY,TraversalMode::UNORDERED>(bvh, primitives, ray, max_length, diag) :
        traverseBVH<IntersectMode::ANY,TraversalMode::CENTROID>(bvh, primitives, ray,  max_length, diag);

    return hit.distance != INFINITY;
}

bool findAnyIntersectionBVH(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray,
        float max_length) {

    NullCollector diag;
    return findAnyIntersectionBVH(bvh, primitives, ray, max_length, diag); 
}
