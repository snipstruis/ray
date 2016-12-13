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

// Find the closest intersection with any primitive
MiniIntersection findClosestIntersectionBVH(
        BVH const& bvh, 
        BVHNode const& node, 
        Primitives const& primitives, 
        Ray const& ray) {

    // does this ray miss us all together?
    if(rayIntersectsAABB(node.bounds, ray) == INFINITY)
        return MiniIntersection();

    if(!node.isLeaf()) {
        // we are not at a leaf yet - consider both children
        BVHNode const& left = bvh.getNode(node.leftIndex());
        BVHNode const& right = bvh.getNode(node.rightIndex());

        MiniIntersection hitLeft = findClosestIntersectionBVH(bvh, left, primitives, ray);
        MiniIntersection hitRight = findClosestIntersectionBVH(bvh, right, primitives, ray);

        if(hitLeft.distance == INFINITY && hitRight.distance == INFINITY)
            return MiniIntersection();

        if(hitLeft.distance < hitRight.distance) 
            return hitLeft;
        else
            return hitRight;
    }
    else {
        // we are at a leaf - walk all triangles to find an exact hit.
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
}

// find closest triangle intersection for ray
MiniIntersection findClosestIntersectionBVH(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray) {

    return findClosestIntersectionBVH(bvh, bvh.root(), primitives, ray);
}

// return true if ANY triangle is intersected by ray
// (that is, stops as soon as we have a hit)
bool findAnyIntersectionBVH(
        BVH const& bvh, 
        BVHNode const& node, 
        Primitives const& primitives, 
        Ray const& ray,
        float maxDist) {
    // must be looking inside a finite distance
    // with maxDist == INFINITY, we'll get junk results (see below) 
    assert(maxDist < INFINITY);

    // does this ray miss us all together?
    if(rayIntersectsAABB(node.bounds, ray) == INFINITY)
        return false;

    if(!node.isLeaf()) {
        // we are not at a leaf yet - consider both children
        if(findAnyIntersectionBVH(bvh, bvh.getNode(node.leftIndex()), primitives, ray, maxDist))
            return true;
        if(findAnyIntersectionBVH(bvh, bvh.getNode(node.rightIndex()), primitives, ray, maxDist))
            return true;

        return false; // no hit
    }
    else {
        // at a leaf - walk triangles and test for a hit.
        for(unsigned int i = node.first(); i < (node.first() + node.count); i++) {
            assert(i < bvh.indicies.size());
            unsigned int triangleIndex = bvh.indicies[i];
            Triangle const& t = primitives.triangles[triangleIndex];
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

    return findAnyIntersectionBVH(bvh, bvh.root(), primitives, ray, maxDist);
}
