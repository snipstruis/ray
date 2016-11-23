#pragma once
#include <math.h>
#include "basics.h"
#include "stdio.h"

struct Primitive{
    glm::vec3 pos;
    virtual float distance(const& Ray) = 0;
    virtual glm::vec3 normal(const& glm::vec3 position) = 0;
};

struct Plane:Primitive{
    glm::vec3 norm;
    virtual float distance(const& Ray ray){
        assert(glm::length(ray.direction)<(1+1e-6f));
        assert(glm::length(ray.direction)>(1-1e-6f));
        float denom = glm::dot(-norm,ray.direction);
        if(denom> 1e-6f){
            float dist = glm::dot(pos-ray.origin, -norm)/denom;
            if(dist>0.f) return dist;
        }
        return INFINITY;
    };
    virtual glm::vec3 normal(const& glm::vec3 position){return norm;}
};

struct OutSphere:Primitive{
    float radius;
    // stolen from Jacco's slide
    virtual float distance(const& Ray ray){
        glm::vec3 c = pos - ray.origin;
        float t = glm::dot( c, ray.direction);
        glm::vec3 q = c - t * ray.direction;
        float p2 = glm::dot( q, q ); 
        float r2 = radius*radius;
        if (p2 > r2) return INFINITY;
        t -= sqrt( r2 - p2 );
        return t>0?t:INFINITY; // no hit if behind ray start
    };
    virtual glm::vec3 normal(const& glm::vec3 position){return glm::normalize(position-pos);}
};

