#pragma once

#include "primitive.h"
#include "scene.h"
#include "debug_print.h"

#include "glm/vec3.hpp"

#include <vector>

struct AABB {
    glm::vec3 a,b;
};

inline std::ostream& operator<<(std::ostream& os, const AABB& aabb) {
    os << aabb.a << " -> " << aabb.b;
    return os;
}

struct BVHNode {
    BVHNode(): leftFirst(0), count(0) {}

    AABB bounds;
    std::uint32_t leftFirst;
    std::uint32_t count;
};

// check sizes are as expected - prevent accidental cache performance degredation
static_assert(sizeof(AABB) == 24, "AABB size");
static_assert(sizeof(BVHNode) == 32, "BVHNode size");

struct BVH {
    BVH(): root(nullptr) {} 

    BVHNode* root;
    std::vector<unsigned int> indicies;
};


// find the AABB for @count triangles, starting at @start
inline void FindAABB(AABB& result, TriangleSet const& triangles, unsigned int start, unsigned int count) {
    assert(count >= 1);
    assert(start + count <= triangles.size());

    result.a[0] = result.b[0] = triangles[start].v[0][0];
    result.a[1] = result.b[1] = triangles[start].v[0][1];
    result.a[2] = result.b[2] = triangles[start].v[0][2];

    for(unsigned int i = start; i < (start + count); i++) {
        for(unsigned int j = 0; j < 3; j++) {

            result.a[0] = std::min(result.a[0], triangles[i].v[j][0]);
            result.a[1] = std::min(result.a[1], triangles[i].v[j][1]);
            result.a[2] = std::min(result.a[2], triangles[i].v[j][2]);

            result.b[0] = std::max(result.b[0], triangles[i].v[j][0]);
            result.b[1] = std::max(result.b[1], triangles[i].v[j][1]);
            result.b[2] = std::max(result.b[2], triangles[i].v[j][2]);
        }
    }
}

// Build a bad, but valid, BVH. This will have a single root node containing all triangles,
// so traversing it will degrade to a linear search. 
// Useful for testing worst case scenarios, or feeling bad about yourself.
inline BVH* buildStupidBVH(Scene& s) {
    BVH* bvh = new BVH;

    bvh->root = new BVHNode;
    bvh->root->leftFirst = 0;
    bvh->root->count = s.primitives.triangles.size();
    bvh->indicies.resize(s.primitives.triangles.size());

    for (unsigned int i = 0; i < s.primitives.triangles.size(); i++)
        bvh->indicies[i] = i;

    FindAABB(bvh->root->bounds, s.primitives.triangles, 0, s.primitives.triangles.size());

    std::cout << "AABB " << bvh->root->bounds << std::endl;
    return bvh;
}

// Try to build the best possible BVH; i.e., Favor optimal traversals, at the expense of slower build time. 
inline BVH* buildStaticBVH(Scene& s) {
    return nullptr;
}

inline BVH* buildBVH(Scene& s) {
    return buildStupidBVH(s);
}

#if 0
void constructBVH(std::vector<Triangle> const& primitives)  {
	// create index array
	indices = new uint[N];

	for(int i = 0; i < N; i++) 
		indices[i] = i;

	// allocate BVH root node
	root = new BVHNode();

	// subdivide root node
	root->first = 0;
	root->count = N;
	root->bounds = CalculateBounds( primitives, root->first, root->count ); 
	root->Subdivide();
}

void Subdivide() {
	if (count < 3) 
		return;

	this.left = new BVHNode();
	this.right = new BVHNode();

	Partition();
	this.left->Subdivide();
	this.right->Subdivide();
	this.isLeaf = false;
}

void Traverse(Ray const& r)
{
	if(!r.Intersects(bounds)) 
		return;
   	if (isleaf()) {	
		IntersectPrimitives();
    }
	else {
      pool[left].Traverse( r );
      pool[left + 1].Traverse( r );
} }

#endif
