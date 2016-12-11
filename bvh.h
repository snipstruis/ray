#pragma once

#include "primitive.h"
#include "scene.h"
#include "debug_print.h"

#include "glm/vec3.hpp"

#include <vector>

// axis-aligned bounding box
// members not named min/max to avoid clashes with library functions
struct AABB {
    glm::vec3 low, high;
};

inline std::ostream& operator<<(std::ostream& os, const AABB& a) {
    os << a.low << " -> " << a.high;
    return os;
}

struct BVHNode {
    BVHNode(): leftFirst(0), count(0) {}
    BVHNode(unsigned int _leftFirst, unsigned int _count): leftFirst(_leftFirst), count(_count) {}

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
static_assert(sizeof(AABB) == 24, "AABB size");
static_assert(sizeof(BVHNode) == 32, "BVHNode size");

struct BVH {
    BVH() {
        nodes.resize(1); // ensure at least root exists
    } 

    BVH(unsigned int nodeCount) {
        nodes.resize(nodeCount); 
    } 

    BVHNode const& getNode(unsigned int index) const {
        assert(index < nodes.size());
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

    std::vector<BVHNode> nodes;
    std::vector<unsigned int> indicies;
};


// find the AABB for @count triangles, starting at @start
inline void FindAABB(AABB& result, TriangleSet const& triangles, unsigned int start, unsigned int count) {
    assert(count >= 1);
    assert(start + count <= triangles.size());

    // need a starting min/max value. could set this to +/- INF. for now we'll use the zeroth vertex..
    // this is not ideal, but we'll be rewriting this in some kind of SIMD way later anyway.
    result.low[0] = result.high[0] = triangles[start].v[0][0];
    result.low[1] = result.high[1] = triangles[start].v[0][1];
    result.low[2] = result.high[2] = triangles[start].v[0][2];

    for(unsigned int i = start; i < (start + count); i++) {
        for(unsigned int j = 0; j < 3; j++) {
            result.low[0] = std::min(result.low[0], triangles[i].v[j][0]);
            result.low[1] = std::min(result.low[1], triangles[i].v[j][1]);
            result.low[2] = std::min(result.low[2], triangles[i].v[j][2]);

            result.high[0] = std::max(result.high[0], triangles[i].v[j][0]);
            result.high[1] = std::max(result.high[1], triangles[i].v[j][1]);
            result.high[2] = std::max(result.high[2], triangles[i].v[j][2]);
        }
    }
}

// Build a bad, but valid, BVH. This will have a single root node containing all triangles,
// so traversing it will degrade to a linear search. 
// Useful for testing worst case scenarios, or feeling bad about yourself.
inline BVH* buildStupidBVH(Scene& s) {
    BVH* bvh = new BVH;

    bvh->root().leftFirst = 0;
    bvh->root().count = s.primitives.triangles.size();

    bvh->indicies.resize(s.primitives.triangles.size());

    for (unsigned int i = 0; i < s.primitives.triangles.size(); i++)
        bvh->indicies[i] = i;

    FindAABB(bvh->root().bounds, s.primitives.triangles, 0, s.primitives.triangles.size());

    std::cout << "AABB " << bvh->root().bounds << std::endl;
    return bvh;
}


// Try to build the best possible BVH; i.e., Favor optimal traversals, at the expense of slower build time. 
inline BVH* buildStaticBVH(Scene& s) {
    assert(false); //unimplemented
    return nullptr;
}

inline BVH* buildBVH(Scene& s) {
    return buildStupidBVH(s);
}

// Does a ray intersect the BVH node? Returns distance to intersect, or INFINITY if no intersection
float rayIntersectsBVH(BVHNode const& node, Ray const& ray) {

    return INFINITY;
}

// Find the closest intersection with any primitive
Intersection findClosestIntersectionBVH(
        BVH const& bvh, 
        BVHNode const& node, 
        Primitives const& primitives, 
        Ray const& ray) {

    if(!node.isLeaf()) {
        // we are not at a leaf yet - consider both children
        BVHNode const& left = bvh.getNode(node.leftIndex());
        BVHNode const& right = bvh.getNode(node.rightIndex());

        float distLeft = rayIntersectsBVH(left, ray);
        float distRight = rayIntersectsBVH(right, ray);

        if(distLeft == INFINITY && distRight == INFINITY)
            return Intersection(INFINITY);

        Intersection hit;

        if(distLeft < distRight) {
            hit = findClosestIntersectionBVH(bvh, left, primitives, ray);
            if(hit.distance < INFINITY)
                return hit;
    
            if(distRight < INFINITY)
                return findClosestIntersectionBVH(bvh, right, primitives, ray);

            return INFINITY;
        }
        else {
            hit = findClosestIntersectionBVH(bvh, right, primitives, ray);
            if(hit.distance < INFINITY)
                return hit;
    
            if(distLeft< INFINITY)
                return findClosestIntersectionBVH(bvh, left, primitives, ray);

            return INFINITY;
        }
    }
    else {
        // we are at a leaf - walk all triangles to find an exact hit.
        Intersection hit = Intersection(INFINITY);

        for(unsigned int i = node.first(); i < (node.first()+ node.count); i++) {
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
