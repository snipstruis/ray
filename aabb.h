#pragma once

#include "basics.h"
#include "primitive.h"

#include "glm/vec3.hpp"
#include "glm/gtx/io.hpp"

#include <cassert>
#include <ostream>

// axis-aligned bounding box
// members not named min/max to avoid clashes with library functions
struct AABB {
    void sanityCheck() const {
        assert(low[0] <= high[0]);
        assert(low[1] <= high[1]);
        assert(low[2] <= high[2]);
    }

    glm::vec3 low, high;
};

static_assert(sizeof(AABB) == 24, "AABB size");

inline std::ostream& operator<<(std::ostream& os, const AABB& a) {
    os << a.low << ":" << a.high;
    return os;
}

// find the AABB for @count triangles, starting at @start
// uses BVH-style indirect mapping in indicies
inline void calcAABBIndirect(AABB& result, 
        TriangleSet const& triangles,
        TriangleMapping const& indicies, 
        unsigned int start, 
        unsigned int count) {

    assert(count >= 1);
    assert(start + count <= triangles.size());

    // need a starting min/max value. could set this to +/- INF. for now we'll use the zeroth vertex..
    // this is not ideal, but we'll be rewriting this in some kind of SIMD way later anyway.
    result.low[0] = result.high[0] = triangles[start].v[0][0];
    result.low[1] = result.high[1] = triangles[start].v[0][1];
    result.low[2] = result.high[2] = triangles[start].v[0][2];

    for(unsigned int i = start; i < (start + count); i++) {
        Triangle const& t = triangles[indicies[i]];

        for(unsigned int j = 0; j < 3; j++) {
            result.low[0] = std::min(result.low[0], t.v[j][0]);
            result.low[1] = std::min(result.low[1], t.v[j][1]);
            result.low[2] = std::min(result.low[2], t.v[j][2]);

            result.high[0] = std::max(result.high[0], t.v[j][0]);
            result.high[1] = std::max(result.high[1], t.v[j][1]);
            result.high[2] = std::max(result.high[2], t.v[j][2]);
        }
    }

    result.sanityCheck();
}

// find an AABB that surrounds 2x existing ones
inline void combineAABB(AABB& result, AABB const& a, AABB const& b) {
    a.sanityCheck();
    b.sanityCheck();

    result.low[0] = std::min(a.low[0], b.low[0]);
    result.low[1] = std::min(a.low[1], b.low[1]);
    result.low[2] = std::min(a.low[2], b.low[2]);

    result.high[0] = std::max(a.high[0], b.high[0]);
    result.high[1] = std::max(a.high[1], b.high[1]);
    result.high[2] = std::max(a.high[2], b.high[2]);

    result.sanityCheck();
}

// does @outer fully contain @inner?
inline bool containsAABB(AABB const& outer, AABB const& inner) {
    inner.sanityCheck();
    outer.sanityCheck();

    return 
        outer.low[0] <= inner.low[0] && 
        outer.low[1] <= inner.low[1] && 
        outer.low[2] <= inner.low[2] && 
        outer.high[0] >= inner.high[0] && 
        outer.high[1] >= inner.high[1] && 
        outer.high[2] >= inner.high[2];
}

// Does a ray intersect the BVH node? Returns distance to intersect, or INFINITY if no intersection
// based on http://gamedev.stackexchange.com/questions/18436/most-efficient-aabb-vs-ray-collision-algorithms
float rayIntersectsAABB(AABB const& a, Ray const& ray) {
    // FIXME: as this gets called a lot (with the same ray) move these 3 divides out to the calling fn
	float dx = 1.0f / ray.direction[0];
	float dy = 1.0f / ray.direction[1];
	float dz = 1.0f / ray.direction[2];

	float t1 = (a.low[0] - ray.origin[0]) * dx;
	float t2 = (a.high[0] - ray.origin[0]) * dx;

	float t3 = (a.low[1] - ray.origin[1]) * dy;
	float t4 = (a.high[1] - ray.origin[1]) * dy;

	float t5 = (a.low[2] - ray.origin[2]) * dz;
	float t6 = (a.high[2] - ray.origin[2]) * dz;

	float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
	float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

	// if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behind us
	if (tmax < 0)
    	return INFINITY;

	// if tmin > tmax, ray doesn't intersect AABB
	if (tmin > tmax)
    	return INFINITY;

    return tmin;
}


