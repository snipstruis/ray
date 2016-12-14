#pragma once

#include "aabb.h"
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
};

// for a leaf - walk all triangles to find an exact hit.
MiniIntersection findIntersectingTriangle(
        BVH const& bvh, 
        BVHNode const& node, 
        Primitives const& primitives, 
        Ray const& ray) {
    MiniIntersection hit;

    for(unsigned int i = node.first(); i < (node.first() + node.count); i++) {
        assert(i < bvh.indicies.size());
        unsigned int triangleIndex = bvh.indicies[i];
        Triangle const& t = primitives.triangles[triangleIndex];
        float distance = moller_trumbore(t, ray);

        if(distance > 0 && distance < hit.distance) {
            hit.distance = distance;
            hit.triangle = triangleIndex;
        }
    }
    return hit;
}

// Find the closest intersection with any primitive
MiniIntersection findClosestIntersectionBVH(
        BVH const& bvh, 
        BVHNode const& node, 
        Primitives const& prims, 
        Ray const& ray,
        glm::vec3 const& rayInvDir) {

    // does this ray miss us all together?
    if(rayIntersectsAABB(node.bounds, ray.origin, rayInvDir) == INFINITY)
        return MiniIntersection();

    if(!node.isLeaf()) {
        // we are not at a leaf yet - consider both children
        auto hitLeft = findClosestIntersectionBVH(bvh, bvh.getNode(node.leftIndex()), prims, ray, rayInvDir);
        auto hitRight = findClosestIntersectionBVH(bvh, bvh.getNode(node.rightIndex()), prims, ray, rayInvDir);

        if(hitLeft.distance == INFINITY && hitRight.distance == INFINITY)
            return MiniIntersection();

        if(hitLeft.distance < hitRight.distance) 
            return hitLeft;
        else
            return hitRight;
    }
    else {
        return findIntersectingTriangle(bvh, node, prims, ray);
    }
}

// find closest triangle intersection for ray
MiniIntersection findClosestIntersectionBVH(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray) {

    // calculate 1/direction here once, as it's used repeatedly throughout the recursive chain
	glm::vec3 invDirection(1.0f/ray.direction[0], 1.0f/ray.direction[1], 1.0f/ray.direction[2]);

    return findClosestIntersectionBVH(bvh, bvh.root(), primitives, ray, invDirection);
}

// return true if ANY triangle is intersected by ray
// (that is, stops as soon as we have a hit)
bool findAnyIntersectionBVH(
        BVH const& bvh, 
        BVHNode const& node, 
        Primitives const& prims, 
        Ray const& ray,
        glm::vec3 const& rayInvDirection,
        float maxDist) {
    // must be looking inside a finite distance
    // with maxDist == INFINITY, we'll get junk results (see below) 
    assert(maxDist < INFINITY);

    // does this ray miss us all together?
    if(rayIntersectsAABB(node.bounds, ray.origin, rayInvDirection) == INFINITY)
        return false;

    if(!node.isLeaf()) {
        // we are not at a leaf yet - consider both children
        if(findAnyIntersectionBVH(bvh, bvh.getNode(node.leftIndex()), prims, ray, rayInvDirection, maxDist))
            return true;
        if(findAnyIntersectionBVH(bvh, bvh.getNode(node.rightIndex()), prims, ray, rayInvDirection, maxDist))
            return true;

        return false; // no hit
    }
    else {
        // at a leaf - walk triangles and test for a hit.
        for(unsigned int i = node.first(); i < (node.first() + node.count); i++) {
            assert(i < bvh.indicies.size());
            unsigned int triangleIndex = bvh.indicies[i];
            Triangle const& t = prims.triangles[triangleIndex];
            float distance = moller_trumbore(t, ray);

            // this will go wrong if maxDist == INFINITY (ie moller_trumbore() returns INFINITY 
            // for no hit), hence the assert at the start of this function
            if(distance > 0 && distance < maxDist) 
                return true;
        }
        return false; // no hit
    }
}

// return true if ANY triangle is intersected by ray
bool findAnyIntersectionBVH(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray,
        float maxDist) {

    // calculate 1/direction here once, as it's used repeatedly throughout the recursive chain
	glm::vec3 invDirection(1.0f/ray.direction[0], 1.0f/ray.direction[1], 1.0f/ray.direction[2]);

    return findAnyIntersectionBVH(bvh, bvh.root(), primitives, ray, invDirection, maxDist);
}

// used to visualise which node/bounds we intersected with
struct BVHIntersectDiag {
    BVHIntersectDiag() : distance(INFINITY), nodeNum(0), depth(0) {}
    BVHIntersectDiag(float _distance, int _nodeNum, int _depth) 
        : distance(_distance), nodeNum(_nodeNum), depth(_depth) {}

    float distance;
    int nodeNum; // index in node array
    int depth; 
};

BVHIntersectDiag findBoundsBVH(
        BVH const& bvh, 
        std::uint32_t nodeNum,
        Primitives const& prims, 
        Ray const& ray,
        glm::vec3 const& rayInvDir,
        int depth) {

    auto const& node = bvh.getNode(nodeNum);

    // does this ray miss us all together?
    if(rayIntersectsAABB(node.bounds, ray.origin, rayInvDir) == INFINITY)
    {
        BVHIntersectDiag result(INFINITY, 0, depth);
        return result;
    }

    if(!node.isLeaf()) {
        // we are not at a leaf yet - consider both children
        auto hitLeft = findBoundsBVH(bvh, node.leftIndex(), prims, ray, rayInvDir, depth+1);
        auto hitRight = findBoundsBVH(bvh, node.rightIndex(), prims, ray, rayInvDir, depth+1);

        if(hitLeft.distance == INFINITY && hitRight.distance == INFINITY){
            if(hitLeft.depth > hitRight.depth)
                return hitLeft;
        }

        if(hitLeft.distance < hitRight.distance) 
            return hitLeft;
        else
            return hitRight;
    }
    else {
        auto hit = findIntersectingTriangle(bvh, node, prims, ray);
        BVHIntersectDiag result(INFINITY, nodeNum, depth);
        if(hit.distance < INFINITY) {
            result.distance = hit.distance;
        }

        return result;
    }
}

BVHIntersectDiag findBoundsBVH(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray) {

    // calculate 1/direction here once, as it's used repeatedly throughout the recursive chain
	glm::vec3 invDirection(1.0f/ray.direction[0], 1.0f/ray.direction[1], 1.0f/ray.direction[2]);

    return findBoundsBVH(bvh, 0, primitives, ray, invDirection, 0);
}
