#pragma once

#include "aabb.h"
#include "bvh.h"
#include "primitive.h"

#include "glm/vec3.hpp"

// minimal intersection result
// note: if dist == INFINITY, triangle is undefined.
struct MiniIntersection {
    MiniIntersection(float _distance, int _triangle) : distance(_distance), triangle(_triangle) {}
    MiniIntersection() : distance(INFINITY) {} 

    // did we hit something? if so, triangle should be defined
    bool hit() const {
        return (distance < INFINITY);
    }

    float distance;         // dist to intersection 
    unsigned int triangle;  // triangle number
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
    // these stats refer to every node visited
    void combineStats(DiagnosticCollector const& other) {
        splitsTraversed += other.splitsTraversed;
        trianglesChecked += other.trianglesChecked;
        leafsChecked += other.leafsChecked;
    }

    // combine node selection from child node - these are stats that just refer to the single 
    // chain from ultimate leaf selected -> root
    void combineSelection(DiagnosticCollector const& other) {
        // node index is an absolute selection
        nodeIndex = other.nodeIndex;
        // leafdepth is, well, depth.. so grow this
        leafDepth = other.leafDepth + 1;
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
    static void incLeafDepth() {}
    static void setNodeIndex(unsigned int nodeIndex) {}
    static void combineStats(NullCollector const& other) {}
    static void combineSelection(NullCollector const& other) {}
};

enum class IntersectMode{
    CLOSEST,
    ANY
};


// for a given BVH leaf node, traverse the triangles and find a hit per IntersectMode
template<IntersectMode MODE, class DiagType>
MiniIntersection traverseTriangles(
        BVH const& bvh, 
        unsigned int nodeIndex, 
        Primitives const& prims, 
        Ray const& ray,
        glm::vec3 const& rayInvDir,
        float const maxDist,
        DiagType& diag) {

    diag.incLeafsChecked();

    BVHNode const& node = bvh.getNode(nodeIndex);
    assert(node.isLeaf());

    // parent should perform bounds check.
    assert(rayIntersectsAABB(node.bounds, ray.origin, rayInvDir) < INFINITY);

    MiniIntersection hit; 

    for(unsigned int i = node.first(); i < (node.first() + node.count); i++) {
        diag.incTrianglesChecked();

        assert(i < bvh.indicies.size());
        unsigned int triangleIndex = bvh.indicies[i];
        TrianglePos const& t = prims.pos[triangleIndex];
        float distance = moller_trumbore(t, ray);

        if (MODE==IntersectMode::ANY){ // if checking for any intersection whatsoever
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

    // parent should perform bounds check before calling us
    assert(rayIntersectsAABB(node.bounds, ray.origin, rayInvDir) < INFINITY);

    if(node.isLeaf()) {
        // at a leaf - walk triangles and test for a hit.
        return traverseTriangles<MODE>(bvh, nodeIndex, prims, ray, rayInvDir, maxDist, diag);
    }

    diag.incSplitsTraversed();

    // ordered / non-ordered traversal?
    if (TRAV==TraversalMode::Ordered){
        // ordered traversal
        glm::vec3 leftCentroid  = centroidAABB(bvh.getNode(node.leftIndex()).bounds);
        glm::vec3 rightCentroid = centroidAABB(bvh.getNode(node.rightIndex()).bounds);

        // find the biggest axis
        glm::vec3 lengths = glm::abs(leftCentroid - rightCentroid);
        int axis = largestElem(lengths);

        bool left_closer = ray.direction[axis] > 0.f;
        int closeIndex  = left_closer?node.leftIndex():node.rightIndex();
        int farIndex = left_closer?node.rightIndex():node.leftIndex();

        // ok we'll now call the 2 AABBs close and far - which doesn't nescessarily mean which one contains
        // our nearest intersection

        DiagType diagClose, diagFar;
        MiniIntersection closeHit, farHit;

        // get distance to far AABB
        float distCloseAABB = rayIntersectsAABB(bvh.getNode(closeIndex).bounds, ray.origin, rayInvDir);
        float distFarAABB = rayIntersectsAABB(bvh.getNode(farIndex).bounds, ray.origin, rayInvDir);

        // intersects with close bounds?
        if(distCloseAABB < INFINITY) {
            // find intersection in close node.
            closeHit = traverseBVH<MODE,TRAV>(bvh, closeIndex, prims, ray, rayInvDir, maxDist, diagClose);
            diag.combineStats(diagClose);

            // if closest intersection is closer than the far aabb, we win
            // Note that this also covers the case that distFarAABB == INFINITY
            if(closeHit.distance < distFarAABB){
                diag.combineSelection(diagClose);
                return closeHit;
            }
        }

        // intersects with far bounds?
        if(distFarAABB < INFINITY) {
            farHit = traverseBVH<MODE,TRAV>(bvh, farIndex, prims, ray, rayInvDir, maxDist, diagFar);
            diag.combineStats(diagFar);

            if(closeHit.distance < farHit.distance) {
                diag.combineSelection(diagClose);
                return closeHit;
            } else {
                diag.combineSelection(diagFar);
                return farHit;
            }
        }

        return MiniIntersection(); // complete miss
    }else{ 
        // unordered traversal
        //
        MiniIntersection hitLeft, hitRight;
        DiagType diagLeft, diagRight;

        if(rayIntersectsAABB(bvh.getNode(node.leftIndex()).bounds, ray.origin, rayInvDir) < INFINITY) {
            hitLeft = traverseBVH<MODE,TRAV>(bvh, node.leftIndex(), prims, ray, rayInvDir, maxDist, diagLeft);
            diag.combineStats(diagLeft);
        }

        if(rayIntersectsAABB(bvh.getNode(node.rightIndex()).bounds, ray.origin, rayInvDir) < INFINITY) {
            hitRight = traverseBVH<MODE,TRAV>(bvh, node.rightIndex(), prims, ray, rayInvDir, maxDist, diagRight);
            diag.combineStats(diagRight);
        }

        // complete miss?
        if(!hitLeft.hit() && !hitRight.hit()) {
            return MiniIntersection();
        }

        if(hitLeft.distance < hitRight.distance) {
            diag.combineSelection(diagLeft);
            return hitLeft;
        } else {
            diag.combineSelection(diagRight);
            return hitRight;
        }
    }
}

// entry point for the main tree walk
template<IntersectMode MODE, TraversalMode TRAV, class DiagnosticCollectorType>
MiniIntersection traverseBVH(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray,
        float const maxDist,
        DiagnosticCollectorType& diag) {

    // calculate 1/direction here once, as it's used repeatedly throughout the recursive chain
    glm::vec3 rayInvDir(1.0f/ray.direction[0], 1.0f/ray.direction[1], 1.0f/ray.direction[2]);

    BVHNode const& node = bvh.getNode(0);

    if(rayIntersectsAABB(node.bounds, ray.origin, rayInvDir) < INFINITY)
        return traverseBVH<MODE,TRAV>(bvh, 0, primitives, ray, rayInvDir, maxDist, diag);

    // missed bounds all together
    return MiniIntersection();
}

template<class DiagnosticCollectorType>
MiniIntersection findClosestIntersectionBVH(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray,
        DiagnosticCollectorType& diag,
        TraversalMode traversalMode) {

    return (traversalMode==TraversalMode::Unordered) ?
        traverseBVH<IntersectMode::CLOSEST, TraversalMode::Unordered>(bvh, primitives, ray, 0.f, diag) :
        traverseBVH<IntersectMode::CLOSEST, TraversalMode::Ordered>(bvh, primitives, ray, 0.f, diag);
}

MiniIntersection findClosestIntersectionBVH(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray,
        TraversalMode traversalMode) {

    NullCollector diag;
    return findClosestIntersectionBVH(bvh, primitives, ray, diag, traversalMode); 
}

template<class DiagnosticCollectorType>
bool findAnyIntersectionBVH(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray,
        float maxLength,
        DiagnosticCollectorType& diag,
        TraversalMode traversalMode) {
    
    MiniIntersection hit = (traversalMode==TraversalMode::Unordered) ?
        traverseBVH<IntersectMode::ANY, TraversalMode::Unordered>(bvh, primitives, ray, maxLength, diag) :
        traverseBVH<IntersectMode::ANY, TraversalMode::Ordered>(bvh, primitives, ray, maxLength, diag);

    return hit.hit(); 
}

bool findAnyIntersectionBVH(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray,
        float maxLength,
        TraversalMode traversalMode) {

    NullCollector diag;
    return findAnyIntersectionBVH(bvh, primitives, ray, maxLength, diag, traversalMode); 
}

