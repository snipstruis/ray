#pragma once
#include "utils.h"
#include "basics.h"
#include "primitive.h"
#include "scene.h"
#include <memory>

// this allows us to toggle rendering features on and off at runtime
enum : unsigned {
    MODE_diffuse      = 1<<0,
    MODE_shadow       = 1<<1,
    MODE_specular     = 1<<2,
    MODE_transparancy = 1<<3,
    MODE_ALL          = 0xFFFFFFFF
};

Color trace(Ray ray, 
            std::vector<std::unique_ptr<Primitive>> const& primitives, 
            std::vector<PointLight> const& lights,
            unsigned mode = 0){
    if(ray.ttl<=0) return Color(0,0,0);

    // hit check
    Primitive const* closest_object = nullptr;
    float dist = INFINITY;
    for(auto const& p: primitives){
        float d = p->distance(ray);
        if(d<dist){
            dist = d;
            closest_object = &*p;
        }
    }
    if(dist==INFINITY) return Color(0, 0, 0);
    Color color = closest_object->mat.color;

    // shadow
    glm::vec3 impact = ray.origin + ray.direction * dist;
    if(mode&MODE_shadow){
        for(PointLight const& light: lights){
            glm::vec3 impact_to_light = light.pos - impact;
            float light_distance = glm::length(impact_to_light);
            glm::vec3 light_direction = glm::normalize(impact_to_light);
            Ray shadowRay = Ray(impact, light_direction, ray.ttl-1);
            bool hit = true;
            for(auto const& primitive: primitives){
                
            }
        }
    }   
    
    // diffuse
    if(mode&MODE_diffuse){
        glm::vec3 normal = closest_object->normal(impact);
        color *= glm::dot(normal, ray.direction);
    }


    return color;
}

