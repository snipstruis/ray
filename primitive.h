#pragma once
#include <math.h>
#include "basics.h"
#include "stdio.h"

struct Primitive{
    glm::vec3 pos;
    Material  mat;
    virtual float distance(Ray) = 0;
};

struct Plane:Primitive{
    glm::vec3 normal;
    virtual float distance(Ray ray){
        assert(glm::length(ray.direction)<(1+1e-6f));
        assert(glm::length(ray.direction)>(1-1e-6f));
        float denom = glm::dot(-normal,ray.direction);
        if(denom> 1e-6f){
            float dist = glm::dot(pos-ray.origin, -normal)/denom;
            if(dist>0.f) return dist;
        }
        return INFINITY;
    };
};

struct OutSphere:Primitive{
    float radius;
    // stolen from Jacco's slide
    virtual float distance(Ray ray){
        glm::vec3 c = pos - ray.origin;
        float t = glm::dot( c, ray.direction);
        glm::vec3 q = c - t * ray.direction;
        float p2 = glm::dot( q, q ); 
        float r2 = radius*radius;
        if (p2 > r2) return INFINITY;
        t -= sqrt( r2 - p2 );
        return t>0?t:INFINITY; // no hit if behind ray start
    };
};

