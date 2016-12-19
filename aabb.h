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
    AABB(){
        low  = {INFINITY,INFINITY,INFINITY};
        high = {-INFINITY,-INFINITY,-INFINITY};
    }
    AABB& operator=(AABB const& bb){low=bb.low;high=bb.high;return *this;}
    AABB(glm::vec3 l, glm::vec3 h):low(l),high(h){}
    void sanityCheck() const {
        assert(low[0] <= high[0]);
        assert(low[1] <= high[1]);
        assert(low[2] <= high[2]);
    }

    glm::vec3 low, high;
};

static_assert(sizeof(AABB) == 24, "AABB size");

inline std::ostream& operator<<(std::ostream& os, const AABB& a) {
    float vol = (a.high[0] - a.low[0]) * (a.high[1] - a.low[1]) * (a.high[2] - a.low[2]);
    os << a.low << ":" << a.high << " volume " << vol ;
    return os;
}

// find the AABB for @count triangles, starting at @start
// uses BVH-style indirect mapping in indicies
inline void calcAABBIndirect(AABB& result, 
        TrianglePosSet const& triangles,
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
        TrianglePosition const& t = triangles[indicies[i]];

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

inline AABB unionAABB(AABB const& a, AABB const& b){
    return {glm::min(a.low,b.low),glm::max(a.high,b.high)};
}

inline AABB unionPoint(AABB const& a, glm::vec3 b){
    return {glm::min(a.low,b),glm::max(a.high,b)};
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

inline bool containsTriangle(AABB const& outer, TrianglePosition const& t) {
    outer.sanityCheck();
    bool result = true;

    for(unsigned int i = 0; i < 3; i++) {
        for(unsigned int j = 0; j < 3; j++) {
            float val = t.v[i][j];
            result &= (val >= outer.low[j]);
            result &= (val <= outer.high[j]);
        }
    }

    return result;
}

// Does a ray intersect the BVH node? Returns distance to intersect, or INFINITY if no intersection
// based on http://gamedev.stackexchange.com/questions/18436/most-efficient-aabb-vs-ray-collision-algorithms
// note this doesn't take a ray, as it needs 1/direction. Given that this function gets called repeatedly
// for the same ray, better to do this at the call site.
float rayIntersectsAABB(AABB const& a, glm::vec3 const& rayOrigin, glm::vec3 const& invDirection) {
	float t1 = (a.low[0] - rayOrigin[0]) * invDirection[0];
	float t2 = (a.high[0] - rayOrigin[0]) * invDirection[0];

	float t3 = (a.low[1] - rayOrigin[1]) * invDirection[1];
	float t4 = (a.high[1] - rayOrigin[1]) * invDirection[1];

	float t5 = (a.low[2] - rayOrigin[2]) * invDirection[2];
	float t6 = (a.high[2] - rayOrigin[2]) * invDirection[2];

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

inline float surfaceAreaAABB(AABB const& aabb){
    glm::vec3 d = aabb.high-aabb.low;
    return 2.f*(d.x*d.y + d.x*d.z + d.y*d.z);
}

