#pragma once
#include "utils.h"
#include "basics.h"
#include "stdio.h"
#include <vector>
#include <cmath>

struct Sphere  {
    Sphere(glm::vec3 p, int m, float r):pos(p), mat(m), radius(r){};
    glm::vec3 pos; int mat; float radius;};
struct Plane   {
    Plane(glm::vec3 p, int m, glm::vec3 n):pos(p), mat(m), normal(n){};
    glm::vec3 pos; int mat; glm::vec3 normal;};
struct Triangle{
    Triangle(glm::vec3 a, glm::vec3 b, glm::vec3 c, int m){
        v[0]=a; v[1]=b; v[2]=c; mat=m;
        pos=glm::vec3((a.x+b.x+c.x)/3.f,
                      (a.y+b.y+c.y)/3.f,
                      (a.z+b.z+c.z)/3.f);
    }
    glm::vec3 pos; int mat; glm::vec3 v[3];};

struct Primitives{
    std::vector<Material> materials;
    std::vector<Sphere>   spheres;
    std::vector<Plane>    planes;
    std::vector<Triangle> triangles;
};

// adapted from:
// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
float moller_trumbore( const glm::vec3   v1,  // Triangle vertices
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

struct Intersection{
    Intersection() = default;
    Intersection(float d):distance(d){};
    Intersection(float d, glm::vec3 i, int m, glm::vec3 n):distance(d),impact(i),mat(m),normal(n){};
    float distance;
    glm::vec3 impact;
    int mat;
    glm::vec3 normal;
};

Intersection intersect(Triangle const& t, Ray const& ray){
    auto a=t.v[0], b=t.v[1], c=t.v[2];
    glm::vec3 normal = glm::normalize(glm::cross(b-a,c-a));
    if(glm::dot(ray.direction,normal)>0) return Intersection(INFINITY);

    float dist = moller_trumbore(a, b, c, ray.origin, ray.direction);
    return dist!=INFINITY?
        Intersection(dist, ray.origin + ray.direction * dist, t.mat, normal)
      : Intersection(INFINITY);
}

Intersection intersect(Plane const& p, Ray const& ray){
    assert(glm::length(ray.direction)<(1+1e-6f));
    assert(glm::length(ray.direction)>(1-1e-6f));
    glm::vec3 pos  = p.pos;
    glm::vec3 norm = p.normal;
    float denom = glm::dot(-norm,ray.direction);
    if(denom> 1e-6f){
        float dist = glm::dot(pos-ray.origin, -norm)/denom;
        if(dist>0.f){
            return Intersection(dist,ray.origin + ray.direction * dist, p.mat, norm);
        }
    }
    return Intersection(INFINITY);
}

Intersection intersect(Sphere const& s, Ray const& ray){
    glm::vec3 pos  = s.pos;
    float r = s.radius;
    glm::vec3 c = pos - ray.origin;
    float t = glm::dot( c, ray.direction);
    glm::vec3 q = c - t * ray.direction;
    float p2 = glm::dot( q, q ); 
    float r2 = r*r;
    if (p2 > r2) return Intersection(INFINITY);
    t -= sqrt( r2 - p2 );
    if(t>0){
        glm::vec3 impact = ray.origin + ray.direction * t;
        glm::vec3 norm = glm::normalize(impact-pos);
        return Intersection(t,impact,s.mat,norm);
    }else return Intersection(INFINITY);
}

Intersection findClosestIntersection(Primitives const& primitives, Ray const& ray){
    Intersection hit = Intersection(INFINITY);
    for(auto const& s: primitives.spheres){
        auto check = intersect(s,ray);
        if(check.distance<hit.distance && check.distance>0){
            hit = check;
        }
    }
    for(auto const& p: primitives.planes){
        auto check = intersect(p,ray);
        if(check.distance<hit.distance && check.distance>0){
            hit = check;
        }
    }
    for(auto const& t: primitives.triangles){
        auto check = intersect(t,ray);
        if(check.distance<hit.distance && check.distance>0){
            hit = check;
        }
    }
    return hit;
};
