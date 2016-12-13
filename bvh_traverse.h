#pragma once

#include "aabb.h"
#include "primitive.h"
#include "debug_print.h"

#include "glm/vec3.hpp"

// Find the closest intersection with any primitive
Intersection findClosestIntersectionBVH(
        BVH const& bvh, 
        BVHNode const& node, 
        Primitives const& primitives, 
        Ray const& ray) {

    // does this ray miss us all together?
    if(rayIntersectsAABB(node.bounds, ray) == INFINITY)
        return Intersection(INFINITY);

    if(!node.isLeaf()) {
        // we are not at a leaf yet - consider both children
        BVHNode const& left = bvh.getNode(node.leftIndex());
        BVHNode const& right = bvh.getNode(node.rightIndex());


        Intersection hitLeft = findClosestIntersectionBVH(bvh, left, primitives, ray);
        Intersection hitRight = findClosestIntersectionBVH(bvh, right, primitives, ray);

        if(hitLeft.distance == INFINITY && hitRight.distance == INFINITY)
            return Intersection(INFINITY);

        if(hitLeft.distance < hitRight.distance) 
            return hitLeft;
        else
            return hitRight;
    }
    else {
        // we are at a leaf - walk all triangles to find an exact hit.
        Intersection hit = Intersection(INFINITY);

        for(unsigned int i = node.first(); i < (node.first() + node.count); i++) {
            assert(i < bvh.indicies.size());
            unsigned int triangleIndex = bvh.indicies[i];
            Triangle const& t = primitives.triangles[triangleIndex];
            auto check = intersect(t, ray);

            if(check.distance < hit.distance && check.distance > 0)
                hit = check;
        }

        return hit;
    }
}

// find closest triangle intersection for ray
Intersection findClosestIntersectionBVH(
        BVH const& bvh, 
        Primitives const& primitives, 
        Ray const& ray) {

    return findClosestIntersectionBVH(bvh, bvh.root(), primitives, ray);

    //FIXME: might be able to decompose this a bit more

}

// return true if ANY triangle is intersected by ray
bool findAnyIntersectionBVH(
        BVH const& bvh, 
        BVHNode const& node, 
        Primitives const& primitives, 
        Ray const& ray,
        float maxDist) {

    // FIXME: implement properly
    Intersection i = findClosestIntersectionBVH(bvh, primitives, ray);
    
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
