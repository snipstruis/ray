#pragma once

#include "aabb.h"
#include "primitive.h"
#include "scene.h"
#include "debug_print.h"

#include "glm/vec3.hpp"

#include <vector>

// this file contains the core BVH machinery for storage and traversal.
// It doesn't contain any BVH building code

struct BVHNode {
    BVHNode(): leftFirst(0), count(0) {}
    BVHNode(std::uint32_t _leftFirst, std::uint32_t _count): leftFirst(_leftFirst), count(_count) {}

    bool isLeaf() const {
        return count > 0;
    }

    // only valid for non-leaves
    std::uint32_t leftIndex() const {
        assert(!isLeaf());
        return leftFirst;
    }

    // only valid for non-leaves
    std::uint32_t rightIndex() const {
        assert(!isLeaf());
        return leftFirst + 1;
    }

    // only valid for leaves
    std::uint32_t first() const {
        assert(isLeaf());
        return leftFirst;
    }

    AABB bounds;
    std::uint32_t leftFirst;
    std::uint32_t count;
};

// check sizes are as expected - prevent accidental cache performance degredation
static_assert(sizeof(BVHNode) == 32, "BVHNode size");

struct BVH {
    BVH() : nextFree(2) {
        nodes.resize(1); // ensure at least root exists
    } 

    BVH(unsigned int triangleCount) : nextFree(2) {
        nodes.resize(triangleCount* 2); 
        indicies.resize(triangleCount);
    } 

    BVHNode const& getNode(unsigned int index) const {
        assert(index < nodes.size());
        assert(index < nextFree);
        assert(index != 1); // index 1 is never allocated
        return nodes[index];
    }

    BVHNode& root() {
        assert(nodes.size() > 0);
        return nodes[0];
    }

    BVHNode const& root() const {
        assert(nodes.size() > 0);
        return nodes[0];
    }

    BVHNode& allocNextNode() {
        assert(nextFree < nodes.size());
        return nodes[nextFree++];
    }

    std::uint32_t nodeCount() const {
        assert(nextFree >= 2);
        return nextFree - 1; // includes root node, but skips the empty 1 node
    }

    std::vector<BVHNode> nodes;
    TriangleMapping indicies;
    std::uint32_t nextFree;
};

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

// walk the whole BVH, explode if the bvh is insane. 
// should compile out on release builds
// this is debug code, it's certainly not especially efficient
void sanityCheckBVH(BVH& bvh, TriangleSet const& triangles) {
    // check triangle refs are sane
    for(std::uint32_t i : bvh.indicies) {
        assert(i < triangles.size());
        
        // check uniqueness
        bool found = false;
        for(std::uint32_t j : bvh.indicies) {
            if(found) {
                assert(j != i);
            } 
            else {
                found = (i == j);
            }
        }
    }

    assert(bvh.nodes.size() >= bvh.nodeCount() + 1);

    // check root node
    if (bvh.root().isLeaf()) {
        assert(bvh.nodeCount() == 1);
    }
    else {
        // first non-root node should be at 2
        assert(bvh.root().leftFirst == 2);
        // walk nodes
        for(std::uint32_t i = 2; i < bvh.nodeCount() + 2; i++) {
            auto const& node = bvh.getNode(i);
            if(node.isLeaf()) {
                assert(node.leftFirst < bvh.indicies.size());
                assert(node.leftFirst + node.count <= bvh.indicies.size());
            } 
            else {
                assert(node.leftFirst > i);
                assert(node.leftFirst < bvh.nodeCount() + 2);
            }
        }
    }
    std::cout << "sanity check OK" <<std::endl;
}




