#pragma once
#include "utils.h"
#include "basics.h"
#include "primitive.h"
#include "scene.h"

// this allows us to toggle rendering features on and off at runtime
enum : unsigned {
    MODE_diffuse      = 1<<0,
    MODE_shadow       = 1<<1,
    MODE_specular     = 1<<2,
    MODE_transparancy = 1<<3,
    MODE_ALL          = 0xFFFFFFFF
};

Color trace(Ray ray, 
            std::vector<Primitive>  const& primitives, 
            std::vector<PointLight> const& lights,
            unsigned mode = 0){

    // hit check
    Primitive const* closest_object = nullptr;
    float dist = INFINITY;
    for(Primitive const& p: primitives){
        float d = p.distance(ray);
        if(d<dist){
            dist = d;
            closest_object = &p;
        }
    }
    if(dist==INFINITY) return Color(0,0,0);
    Color color = closest_object->mat.color;

    // diffuse
    glm::vec3 impact = ray.origin + ray.direction * dist;
    if(mode&MODE_diffuse){
        glm::vec3 normal = closest_object->normal(impact);
        color *= glm::dot(normal, ray.direction);
    }

    // shadow
    if(mode&MODE_shadow){

    }
    return color;
}

