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
    AABB() :
        low{INFINITY,INFINITY,INFINITY},
        high{-INFINITY,-INFINITY,-INFINITY}
        {}

    AABB(glm::vec3 const& l, glm::vec3 const& h) : low(l), high(h) {}

    AABB& operator=(AABB const& bb){
        low=bb.low; 
        high=bb.high; 
        return *this;
    }

    void sanityCheck() const {
        assert(low[0] <= high[0]);
        assert(low[1] <= high[1]);
        assert(low[2] <= high[2]);
    }
    
    // get the 3x axis lengths
    glm::vec3 lengths() const {
        sanityCheck();
        return high - low;
    }

    int longestAxis() const {
        glm::vec3 diff = lengths();
        return diff[0] > diff[1] ?  
                  diff[0] > diff[2] ? 0 : 2
                : diff[1] > diff[2] ? 1 : 2;
    }

    glm::vec3 low, high;
};

static_assert(sizeof(AABB) == 24, "AABB size");

// DANGER - uses == operator directly on floats, be aware
bool operator==(AABB const& a, AABB const& b){
    return (a.low==b.low) && (a.high==b.high);
};

inline AABB unionAABB(AABB const& a, AABB const& b){
    return {glm::min(a.low, b.low), glm::max(a.high, b.high)};
}

inline AABB unionPoint(AABB const& a, glm::vec3 b){
    AABB result = AABB{glm::min(a.low,b), glm::max(a.high,b)};
    result.sanityCheck();
    return result;
}

inline float volumeAABB(AABB const& a) {
    return (a.high[0] - a.low[0]) * (a.high[1] - a.low[1]) * (a.high[2] - a.low[2]);
}

inline std::ostream& operator<<(std::ostream& os, const AABB& a) {
    os << a.low << ":" << a.high << " volume " << volumeAABB(a);
    return os;
}

inline glm::vec3 centroidAABB(AABB const& a){
    return 0.5f * (a.high + a.low);
}

// find the AABB for @count triangles, starting at @start
// creates an aabb entirely containing the triangles 
// uses BVH-style indirect mapping in indicies
inline AABB buildAABBExtrema(
        TrianglePosSet const& triangles,
        TriangleMapping const& indicies, 
        unsigned int start, 
        unsigned int count) {

    AABB result;
    // note: assumes AABB is default constructed to -INFINITY/INFINITY

    assert(count >= 1);
    assert(start + count <= triangles.size());

    for(unsigned int i = start; i < (start + count); i++) {
        TrianglePos const& t = triangles[indicies[i]];

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
    return result;
}

// find the AABB for @count triangles, starting at @start
// creates an aabb that surrounds the triangles' centroids
// uses BVH-style indirect mapping in indicies
inline AABB buildAABBCentroid(
        TrianglePosSet const& triangles,
        TriangleMapping const& indicies, 
        unsigned int start, 
        unsigned int count) {

    AABB result;
    // note: assumes AABB is default constructed to -INFINITY/INFINITY

    assert(count >= 1);
    assert(start + count <= triangles.size());

    for(unsigned int i = start; i < (start + count); i++) {
        TrianglePos const& t = triangles[indicies[i]];
        const glm::vec3 centroid = t.getCentroid();
        result = unionPoint(result, centroid);
    }

    result.sanityCheck();
    return result;
}

// does @outer fully contain @inner?
inline bool containsAABB(AABB const& outer, AABB const& inner) {
    inner.sanityCheck();
    outer.sanityCheck();

    return 
        outer.low.x <= inner.low.x && 
        outer.low.y <= inner.low.y && 
        outer.low.z <= inner.low.z && 
        outer.high.x >= inner.high.x && 
        outer.high.y >= inner.high.y && 
        outer.high.z >= inner.high.z;
}

inline bool containsTriangle(AABB const& outer, TrianglePos const& t) {
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
// and Physically Based Rendering (Pharr, Jakob, Humphreys) 3rd ed. pp 129
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

inline float surfaceAreaAABB(AABB const& aabb) {
    glm::vec3 d = aabb.high-aabb.low;
    return 2.0f * (d.x*d.y + d.x*d.z + d.y*d.z);
}

inline AABB triangleBounds(TrianglePos const& t) {
    AABB result;
    for(int i = 0 ; i < 3; i++)
        result = unionPoint(result, t.v[i]);
    result.sanityCheck();
    return result;
}
