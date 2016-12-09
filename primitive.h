#pragma once

#include "utils.h"
#include "basics.h"
#include "material.h"

#include <vector>
#include <cmath>

struct Triangle{
    Triangle(
            glm::vec3 const& _v1, glm::vec3 const& _v2, glm::vec3 const& _v3, 
            glm::vec3 const& _n1, glm::vec3 const& _n2, glm::vec3 const& _n3, 
            int _material) :
        v{_v1, _v2, _v3}, 
        n{_n1, _n2, _n3}, 
        mat(_material) { } 
    glm::vec3 v[3];
    // per-vertex normal
    glm::vec3 n[3];
    // material
    int mat; 
};

struct Primitives{
    std::vector<float> vx[3], vy[3], vz[3];
    std::vector<float> nx[3], ny[3], nz[3];
    std::vector<Material> mat;
    void add_triangle(
            glm::vec3 const& va, glm::vec3 const& vb, glm::vec3 const& vc, 
            glm::vec3 const& na, glm::vec3 const& nb, glm::vec3 const& nc, 
            Material const& material){
        vx[0].push_back(va.x); vy[0].push_back(va.y); vz[0].push_back(va.z);
        vx[1].push_back(vb.x); vy[1].push_back(vb.y); vz[1].push_back(vb.z);
        vx[2].push_back(vc.x); vy[2].push_back(vc.y); vz[2].push_back(vc.z);
        nx[0].push_back(na.x); ny[0].push_back(na.y); nz[0].push_back(na.z);
        nx[1].push_back(nb.x); ny[1].push_back(nb.y); nz[1].push_back(nb.z);
        nx[2].push_back(nc.x); ny[2].push_back(nc.y); nz[2].push_back(nc.z);
        mat.push_back(material);
    }
    inline size_t triangle_count() const { return vx[0].size(); }
};

// result of an intersection calculation
struct Intersection{
    Intersection() = default;
    Intersection(float d):distance(d){};
    Intersection(float d, glm::vec3 i, int m, glm::vec3 n, bool intr)
        :distance(d),impact(i),mat(m),normal(n),internal(intr){};
    float distance;
    glm::vec3 impact;
    int mat;
    glm::vec3 normal;
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
inline float moller_trumbore( 
                       const glm::vec3   v1,  // Triangle vertices
                       const glm::vec3   v2,
                       const glm::vec3   v3,
                       const glm::vec3    o,   // Ray origin
                       const glm::vec3    d) { // Ray direction
    // Find vectors for two edges sharing V1
    glm::vec3 e1 = v2 - v1;
    glm::vec3 e2 = v3 - v1;

    // Begin calculating determinant - also used to calculate u parameter
    glm::vec3 p = glm::cross(d, e2);

    // if determinant is near zero, ray lies in plane of triangle or ray is parallel 
    // to plane of triangle
    float det = glm::dot(e1, p);

    // NOT CULLING
    if(det > -EPSILON && det < EPSILON) 
        return INFINITY;

    float inv_det = 1.f / det;

    // calculate distance from V1 to ray origin
    glm::vec3 t = o - v1;

    // Calculate u parameter and test bound
    float u = glm::dot(t, p) * inv_det;

    // The intersection lies outside of the triangle
    if(u < 0.f || u > 1.f) 
        return INFINITY;

    // Prepare to test v parameter
    glm::vec3 q = glm::cross(t, e1);

    // Calculate V parameter and test bound
    float v = glm::dot(d, q) * inv_det;

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
inline Intersection intersect(Ray const& ray, int const mat,
                              glm::vec3 va, glm::vec3 vb, glm::vec3 vc,
                              glm::vec3 na, glm::vec3 nb, glm::vec3 nc){
    float dist = moller_trumbore(va, vb, vc, ray.origin, ray.direction);

    if(dist==INFINITY) {
        return Intersection(INFINITY);
    } else {
        glm::vec3 hit = ray.origin + ray.direction * dist;

        // smoothing
        glm::vec3 bary = barycentric(hit, va, vb, vc);
        glm::vec3 normal = 
            glm::normalize( bary.x*na + bary.y*nb + bary.z*nc );

        // internal check
        bool internal = glm::dot(ray.direction,normal)>0;
        normal = internal? -normal : normal;

        return Intersection(dist, hit, mat, normal, internal);
    }
}

// find closest intersection with any geometry
inline Intersection findClosestIntersection(Primitives const& p, 
                                            Ray        const& ray) {
    Intersection hit = Intersection(INFINITY);
    size_t triangle_count = p.triangle_count();
    for(int i=0; i<triangle_count; i++){
        auto check = intersect(ray, i,
                glm::vec3(p.vx[0], p.vy[0], p.vz[0]),
                glm::vec3(p.vx[1], p.vy[1], p.vz[1]),
                glm::vec3(p.vx[2], p.vy[2], p.vz[2]),
                glm::vec3(p.nx[0], p.ny[0], p.nz[0]),
                glm::vec3(p.nx[1], p.ny[1], p.nz[1]),
                glm::vec3(p.nx[2], p.ny[2], p.nz[2]));
        if(check.distance < hit.distance && check.distance > 0){
            hit = check;
        }
    }
    return hit;
};

// does ray intersect any geometry ? (stops after first hit)
inline bool findAnyIntersection(Primitives const& p, Ray const& ray, float const max_dist) {
    size_t triangle_count = p.triangle_count();
    for(int i=0; i<triangle_count; i++){
        auto check = intersect(ray, i,
                glm::vec3(p.vx[0], p.vy[0], p.vz[0]),
                glm::vec3(p.vx[1], p.vy[1], p.vz[1]),
                glm::vec3(p.vx[2], p.vy[2], p.vz[2]),
                glm::vec3(p.nx[0], p.ny[0], p.nz[0]),
                glm::vec3(p.nx[1], p.ny[1], p.nz[1]),
                glm::vec3(p.nx[2], p.ny[2], p.nz[2]));
        if(check.distance > 0 && check.distance < max_dist)
            return true;
    }
    return false;
};
