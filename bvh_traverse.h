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
struct BVHIntersectDiag {
    BVHIntersectDiag() : splitsTraversed(0), 
        trianglesChecked(0), leafsChecked(0){}
    int splitsTraversed;
    int trianglesChecked;
    int leafsChecked;
};

// Find the closest intersection with any primitive
enum{
    CLOSEST,
    ANY,
    DIAG
};

template<int MODE>
MiniIntersection traverseBVH(
        BVH const& bvh, 
        int nodeIndex, 
        Primitives const& prims, 
        Ray const& ray,
        glm::vec3 const& rayInvDir,
        float const maxDist,
        BVHIntersectDiag* diag) {
    auto const& node = bvh.getNode(nodeIndex);

    // does this ray miss us all together?
    if(rayIntersectsAABB(node.bounds, ray.origin, rayInvDir) == INFINITY){
        return MiniIntersection();
    }

    if(!node.isLeaf()) {
        if constexpr(MODE==DIAG) diag->splitsTraversed++; 
        // we are not at a leaf yet - consider both children
        auto hitLeft  = traverseBVH<MODE>(bvh, node.leftIndex(),  prims, ray, rayInvDir, maxDist, diag);
        auto hitRight = traverseBVH<MODE>(bvh, node.rightIndex(), prims, ray, rayInvDir, maxDist, diag);

        if(hitLeft.distance == INFINITY && hitRight.distance == INFINITY){
            if(MODE==DIAG) hitLeft.leafDepth++;
            return MiniIntersection();
        }

        if(hitLeft.distance < hitRight.distance){
            if(MODE==DIAG) hitLeft.leafDepth++;
            return hitLeft;
        } else {
            if(MODE==DIAG) hitRight.leafDepth++;
            return hitRight;
        }
    } else {    
        // at a leaf - walk triangles and test for a hit.
        if constexpr(MODE==DIAG) diag->leafsChecked++;
        MiniIntersection hit;
        for(unsigned int i = node.first(); i < (node.first() + node.count); i++) {
            if constexpr(MODE==DIAG) diag->trianglesChecked++;
            assert(i < bvh.indicies.size());
            unsigned int triangleIndex = bvh.indicies[i];
            TrianglePosition const& t = prims.pos[triangleIndex];
            float distance = moller_trumbore(t, ray);

            if constexpr(MODE==ANY){ // if checking for any intersection whatsoever
                if(distance > 0 && distance < maxDist) return hit;
            }else{ // if we want to find the closest intersection
                if(distance > 0 && distance < hit.distance) {
                    if constexpr(MODE==DIAG) hit.nodeIndex = nodeIndex;
                    hit.distance = distance;
                    hit.triangle = triangleIndex;
                }
            }
        }
        return hit;
    }
}

// find closest triangle intersection for ray
template<int MODE>
MiniIntersection traverseBVH(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray,
        float const maxDist,
        BVHIntersectDiag* diag) {
    // calculate 1/direction here once, as it's used repeatedly throughout the recursive chain
    glm::vec3 invDirection(1.0f/ray.direction[0], 1.0f/ray.direction[1], 1.0f/ray.direction[2]);
    return traverseBVH<MODE>(bvh, 0, primitives, ray, invDirection, maxDist, diag);
}

MiniIntersection findClosestIntersectionBVH(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray){
    return traverseBVH<CLOSEST>(bvh, primitives, ray, 0.f, nullptr);
}

MiniIntersection findClosestIntersectionBVH_DIAG(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray,
        BVHIntersectDiag *diag){
    return traverseBVH<DIAG>(bvh, primitives, ray, 0.f, diag);
}

bool findAnyIntersectionBVH(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray,
        float max_length){
    return traverseBVH<ANY>(bvh, primitives, ray, max_length, nullptr).distance != INFINITY;
}

