#pragma once

#include "glm/vec3.hpp"

#include <vector>

struct AABB {
    glm::vec3 a,b;
};

struct BVHNode {
    AABB bounds;
    std::uint32_t leftFirst;
    std::uint32_t count;
};

// check sizes are as expected, ie 2x per cacheline
static_assert(sizeof(BVHNode) == 32, "BVHNode size");

struct BVH {
    std::vector<BVHNode> pool;	
};


// Build a bad, but valid, BVH. This will have a single root node containing all triangles,
// so traversing it will degrade to a linear search. 
// Useful for testing worst case scenarios, or feeling bad about yourself.
BVH* BuildStupidBVH() {
    return nullptr;
}

// Try to build the best possible BVH; i.e., Favor optimal traversals, at the expense of slower build time. 
BVH* BuildStaticBVH() {
    return nullptr;
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
