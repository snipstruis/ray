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
bool findAnyIntersectionBVH(
        BVH const& bvh, 
        BVHNode const& node, 
        Primitives const& primitives, 
        Ray const& ray,
        float maxDist) {

    // FIXME: implement properly
    MiniIntersection i = findClosestIntersectionBVH(bvh, primitives, ray);
    
    return i.distance < maxDist;
}

// return true if ANY triangle is intersected by ray
bool findAnyIntersectionBVH(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray,
        float maxDist) {

    return findAnyIntersectionBVH(bvh, bvh.root(), primitives, ray, maxDist);
}
