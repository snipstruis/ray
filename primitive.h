#pragma once

#include "utils.h"
#include "basics.h"
#include "material.h"

#include <vector>
#include <cmath>

inline bool SMOOTHING=true;

struct TrianglePosition{
    TrianglePosition(glm::vec3 const& v1, glm::vec3 const& v2, glm::vec3 const& v3)
        : v{v1, v2, v3} { } 

    // FIXME: consider caching or pre-calcuating centoid (but we'll likely redo this structure anyway)
    glm::vec3 getCentroid() const {
        return glm::vec3(
            (v[0][0] + v[1][0] + v[2][0]) / 3.0f,  // x
            (v[0][1] + v[1][1] + v[2][1]) / 3.0f,  // y
            (v[0][2] + v[1][2] + v[2][2]) / 3.0f); // z
    }

    // for a given axis, return the minimum vertex coordinate
    float getMinCoord(unsigned int axis) const {
        assert(axis < 3);
        return std::min(v[0][axis], std::min(v[1][axis], v[2][axis]));
    }

    // for a given axis, return the max vertex coordinate
    float getMaxCoord(unsigned int axis) const {
        assert(axis < 3);
        return std::max(v[0][axis], std::max(v[1][axis], v[2][axis]));
    }

    glm::vec3 v[3];
};

struct Triangle{
    Triangle( glm::vec3 const& n1, glm::vec3 const& n2, glm::vec3 const& n3, int material)
        : n{n1,n2,n3}, mat(material) {}
    // per-vertex normal
    glm::vec3 n[3];
    // material
    int mat; 
};

// we pass these around a lot, so typedef them out
typedef std::vector<Material> MaterialSet;
typedef std::vector<Triangle> TriangleSet;
typedef std::vector<TrianglePosition> TrianglePosSet;

struct Primitives{
    MaterialSet materials;
    TriangleSet triangles;
    TrianglePosSet pos;
};

// result of an intersection calculation
// This is the former Intersection struct - the basic result of an intersection is now in MiniIntersection
// generally one of these will be built after deciding a specific triangle is the closest
struct FancyIntersection{
    FancyIntersection() = default;
    FancyIntersection(glm::vec3 i, int m, glm::vec3 n, bool intr)
        : impact(i), mat(m), normal(n), internal(intr){};

    glm::vec3 impact;   // point of impact
    int mat;            // material at impact
    glm::vec3 normal;   // normal at impact
    bool internal;
};

// adapted from Christer Ericson's Read-Time Collition Detection
inline glm::vec3 barycentric(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c) {
    glm::vec3 v0 = b - a, v1 = c - a, v2 = p - a;
    float d00 = glm::dot(v0, v0);
    float d01 = glm::dot(v0, v1);
    float d11 = glm::dot(v1, v1);
    float d20 = glm::dot(v2, v0);
    float d21 = glm::dot(v2, v1);
    float denom = d00 * d11 - d01 * d01;
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;
    return glm::vec3(u,v,w);
}

// adapted from:
// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
inline float moller_trumbore(TrianglePosition const& tri, Ray const& ray) {
    // Find vectors for two edges sharing V1
    glm::vec3 e1 = tri.v[1] - tri.v[0];
    glm::vec3 e2 = tri.v[2] - tri.v[0];

    // Begin calculating determinant - also used to calculate u parameter
    glm::vec3 p = glm::cross(ray.direction, e2);

    // if determinant is near zero, ray lies in plane of triangle or ray is parallel 
    // to plane of triangle
    float det = glm::dot(e1, p);

    // NOT CULLING
    if(det > -EPSILON && det < EPSILON) 
        return INFINITY;

    float inv_det = 1.f / det;

    // calculate distance from V1 to ray origin
    glm::vec3 t = ray.origin - tri.v[0];

    // Calculate u parameter and test bound
    float u = glm::dot(t, p) * inv_det;

    // The intersection lies outside of the triangle
    if(u < 0.f || u > 1.f) 
        return INFINITY;

    // Prepare to test v parameter
    glm::vec3 q = glm::cross(t, e1);

    // Calculate V parameter and test bound
    float v = glm::dot(ray.direction, q) * inv_det;

    // The intersection lies outside of the triangle
    if(v < 0.f || u + v  > 1.f) 
        return INFINITY;

    float ret = glm::dot(e2, q) * inv_det;

    if(ret > EPSILON)  // ray intersection
        return ret;

    // No hit, no win
    return INFINITY;
}

// compute triangle/ray intersection
// assumes there is an intersection between t & ray already calculated
inline FancyIntersection FancyIntersect(float dist, TrianglePosition const& p, Triangle const& t, Ray const& ray){
    assert(dist < INFINITY);
    glm::vec3 hit = ray.origin + ray.direction * dist;

    glm::vec3 normal;
    // smoothing
    if(SMOOTHING){
        glm::vec3 bary = barycentric(hit, p.v[0], p.v[1], p.v[2]);
        normal = glm::normalize( bary.x*t.n[0] + bary.y*t.n[1] + bary.z*t.n[2] );
    }else{
        normal = (t.n[0]/3.f) + (t.n[1]/3.f) + (t.n[2]/3.f);
    }

    // internal check
    bool internal = glm::dot(ray.direction,normal)>0;
    normal = internal? -normal : normal;

    return FancyIntersection(hit, t.mat, normal, internal);
}
