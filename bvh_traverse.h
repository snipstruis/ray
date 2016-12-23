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
//    int nodeIndex;
    int leafDepth=0;
};

// used to visualise which node/bounds we intersected with
struct DiagnosticCollector {
    DiagnosticCollector() : 
        splitsTraversed(0), 
        trianglesChecked(0), 
        leafsChecked(0), 
        nodeIndex(0), 
        leafDepth(0) 
    {}

    void incSplitsTraversed() {
        splitsTraversed++;
    }

    void incTrianglesChecked() {
        trianglesChecked++;
    }

    void incLeafsChecked() {
        leafsChecked++;
    }

    void setNodeIndex(unsigned int n){
        nodeIndex = n;
    }

    void combine(DiagnosticCollector const& other) {
        splitsTraversed += other.splitsTraversed;
        trianglesChecked += other.trianglesChecked;
        leafsChecked += other.leafsChecked;

        // note: these two don't accumulate
        nodeIndex = nodeIndex;
        leafDepth = leafDepth;
    }

    unsigned int splitsTraversed;
    unsigned int trianglesChecked;   // number of triangles checked for intersection
    unsigned int leafsChecked;       // number of leaf nodes checked
    unsigned int nodeIndex;          // nodeIndex with which we ultimately intersected
    unsigned int leafDepth;
};

// used when we don't care for stats - all code should magically compile away
struct NullCollector {
    static void incSplitsTraversed() {}
    static void incTrianglesChecked() {}
    static void incLeafsChecked() {}
    static void setNodeIndex(unsigned int nodeIndex) {}
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
} traversalMode = TraversalMode::CENTROID;

char const * const traversalstr[] = {
    "unordered",
    "ordered",
    "SHOULD NOT HAPPEN"
};

// for a given BVH leaf node, traverse the triangles and find a hit per IntersectMode
template<IntersectMode MODE, class DiagnosticCollectorType>
MiniIntersection traverseTriangles(
        BVH const& bvh, 
        unsigned int nodeIndex, 
        Primitives const& prims, 
        Ray const& ray,
        glm::vec3 const& rayInvDir,
        float const maxDist,
        DiagnosticCollectorType& diag) {

    BVHNode const& node = bvh.getNode(nodeIndex);
    assert(node.isLeaf());

    MiniIntersection hit; 

    for(unsigned int i = node.first(); i < (node.first() + node.count); i++) {
        diag.incTrianglesChecked();

        assert(i < bvh.indicies.size());
        unsigned int triangleIndex = bvh.indicies[i];
        TrianglePos const& t = prims.pos[triangleIndex];
        float distance = moller_trumbore(t, ray);

        if constexpr(MODE==IntersectMode::ANY){ // if checking for any intersection whatsoever
            if(distance > 0 && distance < maxDist){
                diag.setNodeIndex(nodeIndex);
                hit.distance = distance;
                hit.triangle = triangleIndex;
                return hit;
            }
        }else{ // if we want to find the closest intersection
            if(distance > 0 && distance < hit.distance) {
                diag.setNodeIndex(nodeIndex);
                hit.distance = distance;
                hit.triangle = triangleIndex;
            }
        }
    }
    return hit;
}

template<IntersectMode MODE, TraversalMode TRAV, class DiagnosticCollectorType>
MiniIntersection traverseBVH(
        BVH const& bvh, 
        unsigned int nodeIndex, 
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
        diag.incSplitsTraversed();
        // we are not at a leaf yet - consider both children

        // ordered traversal
        if constexpr(TRAV==TraversalMode::CENTROID){
            glm::vec3 leftCentroid  = centroidAABB(bvh.getNode(node.leftIndex()).bounds);
            glm::vec3 rightCentroid = centroidAABB(bvh.getNode(node.rightIndex()).bounds);

            // find the biggest axis
            glm::vec3 lengths = glm::abs(leftCentroid - rightCentroid);
            int axis = largestElem(lengths);

            bool left_closer = ray.direction[axis] > 0.f;
            int first_index  = left_closer?node.leftIndex():node.rightIndex();
            int second_index = left_closer?node.rightIndex():node.leftIndex();

            // ok we'll now call the 2 AABBs close and far - which doesn't nescessarily mean which one contains
            // our nearest intersection

            // get distance to far AABB
            float distFarAABB = rayIntersectsAABB(bvh.getNode(second_index).bounds, ray.origin, rayInvDir);

            // find intersection in close node. note the first thing the recursive call does is check the AABB
            // so we don't have to bother doing that here. 
            MiniIntersection closeHit = 
                traverseBVH<MODE,TRAV>(bvh, first_index, prims, ray, rayInvDir, maxDist, diag);

            // if we missed the far AABB all together, closest hit is all that's left (may also be INFINITY)
            if(distFarAABB == INFINITY) {
                return closeHit;
            }

            // if closest intersection is closer than the far aabb, we win
            // I think i'm too tired to be sure, but this makes the previous if statement redundant right?
            if(closeHit.distance < distFarAABB){
                return closeHit;
            }

            // Ok, we just don't know. travese farther node, and do it the old fashoned way
            // FIXME: we could make a slight optimisation here - we've already calculated the distance
            // to the far AABB a few lines up, but this call will do it again. shuffle things around to avoid that
            MiniIntersection farHit = 
                traverseBVH<MODE,TRAV>(bvh, second_index,  prims, ray, rayInvDir, maxDist, diag);

            if(closeHit.distance < farHit.distance)
                return closeHit;
            else
                return farHit;

        }else{ // unordered traversal
            auto hitLeft = traverseBVH<MODE,TRAV>(bvh, node.leftIndex(), prims, ray, rayInvDir, maxDist, diag);
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
        diag.incLeafsChecked();
        return traverseTriangles<MODE>(bvh, nodeIndex, prims, ray, rayInvDir, maxDist, diag);
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
