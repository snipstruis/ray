#pragma once
#include <math.h>
#include <glm/glm.hpp>

struct Intersection{
    float distance; // infinite == no hit
    glm::vec3    normal;
};

struct Primitive{
    glm::vec3 pos;
    Material mat;
    virtual Intersection intersects(Ray) = 0;
};

struct Plane:Primitive{
    glm::vec3 normal;
    virtual Intersection intersect(Ray ray){
        assert(glm::length(ray.direction)==0.f);
        float denom = glm::dot(normal,ray.direction);
        if(denom> 1e-6f){
            float dist = glm::dot(ray.origin,normal)/denom;
            if(dist>=0.f) return {dist,normal};
        }
        return {INFINITY,normal};
    };
};

struct Sphere:Primitive{
    float radius;
    virtual Intersection intersect(Ray r){
        return {INFINITY,glm::vec3(0,0,1)};
    };
};

