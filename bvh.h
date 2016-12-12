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

    std::vector<BVHNode> nodes;
//    std::vector<unsigned int> indicies;
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
    {
//        std::cout << "\nMISS" << node.bounds << "\n" << ray << "\n";
        return Intersection(INFINITY);
    }
    else
    {
 //       std::cout << "\nNO " << node.bounds << " " << ray << "\n";
    }


    if(!node.isLeaf()) {
   //     std::cout << "NON LEAF\n";
     //   std::cout << " left = " << node.leftIndex() << " right " << node.rightIndex() <<std::endl;
        // we are not at a leaf yet - consider both children
        BVHNode const& left = bvh.getNode(node.leftIndex());
        BVHNode const& right = bvh.getNode(node.rightIndex());

       // std::cout << " left = " << left.bounds <<std::endl;
        //std::cout << " right= " << right.bounds << std::endl;

        Intersection hitLeft = findClosestIntersectionBVH(bvh, left, primitives, ray);
        Intersection hitRight = findClosestIntersectionBVH(bvh, right, primitives, ray);
//        std::cout << "hitLeft " << hitLeft.distance << " count " << left.count << "\n";
  //      std::cout << "hitRight " << hitRight.distance << " count " << right.count << "\n";

        if(hitLeft.distance == INFINITY && hitRight.distance == INFINITY)
            return Intersection(INFINITY);

    //    std::cout << "|";

        if(hitLeft.distance < hitRight.distance) 
            return hitLeft;
        else
            return hitRight;
    }
    else {
      //  std::cout << "INLEAF\n";
        // we are at a leaf - walk all triangles to find an exact hit.
        Intersection hit = Intersection(INFINITY);

        //std::cout << ".";

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

    auto i = findClosestIntersectionBVH(bvh, bvh.root(), primitives, ray);

//    if(i.distance < INFINITY)
  //      std::cout << "." << std::endl;
    return i;
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
