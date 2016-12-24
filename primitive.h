#pragma once

#include "utils.h"
#include "basics.h"
#include "material.h"

#include "glm/gtx/vector_query.hpp"

#include <vector>
#include <cmath>
#include <cassert>

inline bool SMOOTHING=false;

// Holds the 3 verticies of a triangle.
struct TrianglePos{
    TrianglePos(glm::vec3 const& v1, glm::vec3 const& v2, glm::vec3 const& v3)
        : v{v1, v2, v3} { } 

    float getAverageCoord(unsigned int axis) const {
        assert(axis < 3);
        return (v[0][axis] + v[1][axis] + v[2][axis]) / 3.0f;
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

    glm::vec3 getCentroid() const {
        return glm::vec3(getAverageCoord(0), getAverageCoord(1), getAverageCoord(2));
    }

    glm::vec3 v[3];
};

// 3x per-vertex normals, and ref to a material. Separated from TrianglePos to improve cache performance
struct TriangleExtra{
    TriangleExtra(glm::vec3 const& n1, glm::vec3 const& n2, glm::vec3 const& n3, int _mat)
        : n{n1, n2, n3}, mat(_mat) {
            sanityCheck();
        }

    void sanityCheck() const {
        assert(glm::isNormalized(n[0], EPSILON));
        assert(glm::isNormalized(n[1], EPSILON));
        assert(glm::isNormalized(n[2], EPSILON));
    }

    // per-vertex normal
    glm::vec3 n[3];
    // material
    int mat; 
};

// we pass these around a lot, so typedef them out
typedef std::vector<Material> MaterialSet;
typedef std::vector<TriangleExtra> TriangleExtraSet;
typedef std::vector<TrianglePos> TrianglePosSet;

struct Primitives{
    MaterialSet materials;
    // these next two combined define the world triangles. 
    // They are seperate to improve cache performance. Indicies must line up.
    TrianglePosSet pos;
    TriangleExtraSet extra;
};

// result of an intersection calculation
// This is the former Intersection struct - the basic result of an intersection is now in MiniIntersection
// generally one of these will be built after deciding a specific triangle is the closest
struct FancyIntersection{
    FancyIntersection() = default;
    FancyIntersection(glm::vec3 _impact, int _mat, glm::vec3 _normal, bool _internal)
        : impact(_impact), mat(_mat), normal(_normal), internal(_internal){};

    glm::vec3 impact;   // point of impact
    int mat;            // material at impact
    glm::vec3 normal;   // normal at impact
    bool internal;
};

// adapted from Christer Ericson's Read-Time Collition Detection
inline glm::vec3 barycentric(glm::vec3 p, TrianglePos const& tri) {
    const glm::vec3 v0 = tri.v[1] - tri.v[0];
    const glm::vec3 v1 = tri.v[2] - tri.v[0];
    const glm::vec3 v2 = p - tri.v[0];

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

// compute triangle/ray intersection
// assumes there is an intersection between t & ray already calculated
inline FancyIntersection FancyIntersect(float dist, TrianglePos const& p, TriangleExtra const& t, Ray const& ray){
    assert(dist < INFINITY);
    t.sanityCheck();

    glm::vec3 hit = ray.origin + ray.direction * dist;
    glm::vec3 normal;
    // smoothing
    if(SMOOTHING){
        glm::vec3 bary = barycentric(hit, p);
        normal = glm::normalize(bary.x*t.n[0] + bary.y*t.n[1] + bary.z*t.n[2]);
    }else{
//        normal = (t.n[0]/3.f) + (t.n[1]/3.f) + (t.n[2]/3.f);
        normal = (t.n[0] + t.n[1] + t.n[2]) / 3.0f;
    }

    assert(glm::isNormalized(normal, EPSILON));

    // internal check
    bool internal = glm::dot(ray.direction,normal)>0;
    normal = internal? -normal : normal;

    return FancyIntersection(hit, t.mat, normal, internal);
}

// adapted from:
// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
inline float moller_trumbore(TrianglePos const& tri, Ray const& ray) {
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

