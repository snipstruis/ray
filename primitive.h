#pragma once
#include <math.h>
#include "basics.h"
#include "stdio.h"

struct Primitive{
    glm::vec3 pos;
    virtual float distance(Ray const&) = 0;
    virtual glm::vec3 normal(glm::vec3 const& position) = 0;
};

struct Plane:Primitive{
    glm::vec3 norm;
    virtual float distance(Ray const& ray){
        assert(glm::length(ray.direction)<(1+1e-6f));
        assert(glm::length(ray.direction)>(1-1e-6f));
        float denom = glm::dot(-norm,ray.direction);
        if(denom> 1e-6f){
            float dist = glm::dot(pos-ray.origin, -norm)/denom;
            if(dist>0.f) return dist;
        }
        return INFINITY;
    };
    virtual glm::vec3 normal(glm::vec3 const& position){return norm;}
};

struct OutSphere:Primitive{
    float radius;
    // stolen from Jacco's slide
    virtual float distance(Ray const& ray){
        glm::vec3 c = pos - ray.origin;
        float t = glm::dot( c, ray.direction);
        glm::vec3 q = c - t * ray.direction;
        float p2 = glm::dot( q, q ); 
        float r2 = radius*radius;
        if (p2 > r2) return INFINITY;
        t -= sqrt( r2 - p2 );
        return t>0?t:INFINITY; // no hit if behind ray start
    };
    virtual glm::vec3 normal(glm::vec3 const& position){return glm::normalize(position-pos);}
};

#define EPSILON 1e-6f

// adapted from:
// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
float triangle_intersection( const glm::vec3   v1,  // Triangle vertices
                             const glm::vec3   v2,
                             const glm::vec3   v3,
                             const glm::vec3    o,  //Ray origin
                             const glm::vec3    d  ) { // Ray direction
    //Find vectors for two edges sharing V1
    glm::vec3 e1 = v2 - v1;
    glm::vec3 e2 = v3 - v1;
    //Begin calculating determinant - also used to calculate u parameter
    glm::vec3 p = glm::cross(d, e2);
    //if determinant is near zero, ray lies in plane of triangle or ray is parallel to plane of triangle
    float det = glm::dot(e1, p);
    //NOT CULLING
    if(det > -EPSILON && det < EPSILON) return INFINITY;
    float inv_det = 1.f / det;

    //calculate distance from V1 to ray origin
    glm::vec3 t = o - v1;

    //Calculate u parameter and test bound
    float u = glm::dot(t, p) * inv_det;
    //The intersection lies outside of the triangle
    if(u < 0.f || u > 1.f) return INFINITY;

    //Prepare to test v parameter
    glm::vec3 q = glm::cross(t, e1);

    //Calculate V parameter and test bound
    float v = glm::dot(d, q) * inv_det;
    //The intersection lies outside of the triangle
    if(v < 0.f || u + v  > 1.f) return INFINITY;

    float ret = glm::dot(e2, q) * inv_det;

    if(ret > EPSILON) { //ray intersection
        return ret;
    }

    // No hit, no win
    return INFINITY;
}

