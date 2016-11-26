#pragma once
#include <math.h>
#include "utils.h"
#include "basics.h"
#include "stdio.h"

template<int N>
struct Primitives{
    size_t sphere_end, plane_end, triangle_end, _padding;
    float x[N],y[N],z[N];
    Material mat[N];
    union{
        struct{float r[N];} sphere;
        struct{float nx[N],ny[N],nz[N];} plane;
        struct{float vx[3][N],vy[3][N],vz[3][N];} triangle;
    };
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
    glm::vec3 normal;
    glm::vec3 impact;
};

template<int N>
float intersect_triangle(Primitives<N> const& p, 
                         int id, Ray const& ray, Intersection* out){
 auto a=glm::vec3(p.triangle.vx[0][id], p.triangle.vy[0][id], p.triangle.vz[0][id]),
      b=glm::vec3(p.triangle.vx[1][id], p.triangle.vy[1][id], p.triangle.vz[1][id]),
      c=glm::vec3(p.triangle.vx[2][id], p.triangle.vy[2][id], p.triangle.vz[2][id]);
    float dist = moller_trumbore(a,b,c,ray.origin, ray.direction);
    if(dist!=INFINITY){
        out->impact = ray.origin + ray.direction * dist;
        out->normal = glm::cross(b-a,c-a);
    }
}

template<int N>
float intersect_plane(Primitives<N> const& p, int id, Ray const& ray, Intersection* out){
    assert(glm::length(ray.direction)<(1+1e-6f));
    assert(glm::length(ray.direction)>(1-1e-6f));
    glm::vec3 pos  = glm::vec3(p.x[id],p.y[id],p.z[id]);
    glm::vec3 norm = glm::vec3(p.plane.nx[id],p.plane.ny[id],p.plane.nz[id]);
    float denom = glm::dot(-norm,ray.direction);
    if(denom> 1e-6f){
        float dist = glm::dot(pos-ray.origin, -norm)/denom;
        if(dist>0.f){
            out->impact = ray.origin + ray.direction * dist;
            out->normal = glm::vec3(p.plane.nx[id],p.plane.ny[id],p.plane.nz[id]);
            return dist;
        }
    }
    return INFINITY;
}

template<int N>
float intersect_sphere(Primitives<N> const& p, int id, Ray const& ray, Intersection* out){
    glm::vec3 pos  = glm::vec3(p.x[id],p.y[id],p.z[id]);
    float r = p.sphere.r[id];
    glm::vec3 c = pos - ray.origin;
    float t = glm::dot( c, ray.direction);
    glm::vec3 q = c - t * ray.direction;
    float p2 = glm::dot( q, q ); 
    float r2 = r*r;
    if (p2 > r2) return INFINITY;
    t -= sqrt( r2 - p2 );
    if(t>0){
        out->impact = ray.origin + ray.direction * t;
        out->normal=glm::normalize(out->impact-pos);
        return t;
    }else return INFINITY;
}

template<int N>
float intersect(Primitives<N> const& p, int id, Ray const& ray, Intersection* out){
    assert(out);
    assert(id>0);
    assert(id<p.triangle_end);
    if(id>=p.plane_end){
        return intersect_triangle(p,id,ray,out);
    }else if(id>=p.sphere_end){
        return intersect_plane(p,id,ray,out);
    }else{
        return intersect_sphere(p,id,ray,out);
    }
}


struct Primitive{
    Primitive(glm::vec3 p, Material m):pos(p),mat(m){};
    glm::vec3 pos;
    Material mat;
    virtual float distance(Ray const&) const = 0;
    virtual glm::vec3 normal(glm::vec3 const& position) const = 0;
};

struct Plane:public Primitive{
    Plane(glm::vec3 p, Material m, glm::vec3 n):Primitive(p,m),norm(n){};
    glm::vec3 norm;
    virtual float distance(Ray const& ray) const{
        assert(glm::length(ray.direction)<(1+1e-6f));
        assert(glm::length(ray.direction)>(1-1e-6f));
        float denom = glm::dot(-norm,ray.direction);
        if(denom> 1e-6f){
            float dist = glm::dot(pos-ray.origin, -norm)/denom;
            if(dist>0.f) return dist;
        }
        return INFINITY;
    };
    virtual glm::vec3 normal(glm::vec3 const& position) const {return norm;}
};

struct OutSphere:public Primitive{
    OutSphere(glm::vec3 p, Material m, float r):Primitive(p,m),radius(r){};
    float radius;
    // stolen from Jacco's slide
    virtual float distance(Ray const& ray) const {
        glm::vec3 c = pos - ray.origin;
        float t = glm::dot( c, ray.direction);
        glm::vec3 q = c - t * ray.direction;
        float p2 = glm::dot( q, q ); 
        float r2 = radius*radius;
        if (p2 > r2) return INFINITY;
        t -= sqrt( r2 - p2 );
        return t>0?t:INFINITY; // no hit if behind ray start
    };
    virtual glm::vec3 normal(glm::vec3 const& position)const{return glm::normalize(position-pos);}
};

