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

    // accumulate stats from a child node
    void combineStats(DiagnosticCollector const& other) {
        splitsTraversed += other.splitsTraversed;
        trianglesChecked += other.trianglesChecked;
        leafsChecked += other.leafsChecked;
    }

    // combine selected node stats from child node  
    void combineSelection(DiagnosticCollector const& other) {
        nodeIndex = other.nodeIndex;
        leafDepth = other.leafDepth;
    }

    unsigned int splitsTraversed;
    unsigned int trianglesChecked;   // number of triangles checked for intersection
    unsigned int leafsChecked;       // number of leaf nodes checked
    unsigned int nodeIndex;          // nodeIndex with which we ultimately intersected
    unsigned int leafDepth;          // depth of the leaf node with which we ultimately intersected
};

// used when we don't care for stats - all code should magically compile away
struct NullCollector {
    static void incSplitsTraversed() {}
    static void incTrianglesChecked() {}
    static void incLeafsChecked() {}
    static void setNodeIndex(unsigned int nodeIndex) {}
    static void combineStats(NullCollector const& other) {}
    static void combineSelection(NullCollector const& other) {}
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

    diag.incLeafsChecked();

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
                return hit; // early out!
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

template<IntersectMode MODE, TraversalMode TRAV, class DiagType>
MiniIntersection traverseBVH(
        BVH const& bvh, 
        unsigned int nodeIndex, 
        Primitives const& prims, 
        Ray const& ray,
        glm::vec3 const& rayInvDir,
        float const maxDist,
        DiagType& diag) {
    auto const& node = bvh.getNode(nodeIndex);

    // does this ray miss us all together?
    if(rayIntersectsAABB(node.bounds, ray.origin, rayInvDir) == INFINITY){
        return MiniIntersection();
    }

    if(node.isLeaf()) {
        // at a leaf - walk triangles and test for a hit.
        return traverseTriangles<MODE>(bvh, nodeIndex, prims, ray, rayInvDir, maxDist, diag);
    }

    diag.incSplitsTraversed();

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
        DiagType diagClose;
        MiniIntersection closeHit = 
            traverseBVH<MODE,TRAV>(bvh, first_index, prims, ray, rayInvDir, maxDist, diagClose);

        diag.combineStats(diagClose);

        // if closest intersection is closer than the far aabb, we win
        // Note that this also covers the case that distFarAABB == INFINITY
        if(closeHit.distance < distFarAABB){
            diag.combineSelection(diagClose);
            return closeHit;
        }

        // Ok, we just don't know. travese farther node, and do it the old fashoned way
        // FIXME: we could make a slight optimisation here - we've already calculated the distance
        // to the far AABB a few lines up, but this call will do it again. shuffle things around to avoid that
        DiagType diagFar;
        MiniIntersection farHit = 
            traverseBVH<MODE,TRAV>(bvh, second_index,  prims, ray, rayInvDir, maxDist, diagFar);

        diag.combineStats(diagFar);

        if(closeHit.distance < farHit.distance) {
            diag.combineSelection(diagClose);
            return closeHit;
        } else {
            diag.combineSelection(diagFar);
            return farHit;
        }

    }else{ // unordered traversal
        DiagType diagLeft, diagRight;
        auto hitLeft = traverseBVH<MODE,TRAV>(bvh, node.leftIndex(), prims, ray, rayInvDir, maxDist, diagLeft);
        auto hitRight = traverseBVH<MODE,TRAV>(bvh, node.rightIndex(), prims, ray, rayInvDir, maxDist, diagRight);

        diag.combineStats(diagLeft);
        diag.combineStats(diagRight);

        // complete miss?
        if(hitLeft.distance == INFINITY && hitRight.distance == INFINITY){
            return MiniIntersection();
        }

        if(hitLeft.distance < hitRight.distance){
            diag.combineSelection(diagLeft);
//            if(MODE==IntersectMode::DIAG) hitLeft.leafDepth++;
            return hitLeft;
        } else {
            diag.combineSelection(diagRight);
//            if(MODE==IntersectMode::DIAG) hitRight.leafDepth++;
            return hitRight;
        }
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
