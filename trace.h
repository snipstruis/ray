#pragma once
#include "utils.h"
#include "basics.h"
#include "primitive.h"
#include "scene.h"
#include <memory>

Color trace(Ray ray, 
            std::vector<std::unique_ptr<Primitive>> const& primitives, 
            std::vector<PointLight> const& lights,
            Color const& alpha){
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

    if(dist==INFINITY) return alpha;
    unsigned const mode = closest_object->mat.properties;

    Color color = mode & MAT_lit?
        closest_object->mat.color : Color(0, 0, 0);

    // shadow
    glm::vec3 impact = ray.origin + ray.direction * dist;
    for(PointLight const& light: lights){
        glm::vec3 impact_to_light = light.pos - impact;
        float light_distance = glm::length(impact_to_light);
        glm::vec3 light_direction = glm::normalize(impact_to_light);
        // TODO: when adding transparancy, fix the shadow epsilon,
        // it only works for one-sided surfaces
        Ray shadowRay = Ray(impact-(light_direction*1e-3f), 
                light_direction, 0);

        bool is_hit = false;
        for(auto const& primitive: primitives){
            float d = primitive->distance(shadowRay);
            is_hit = d>0 && d<light_distance ? true : is_hit;
        }

        if(!is_hit){
            if(mode & MAT_diffuse){
                glm::vec3 normal = closest_object->normal(impact);
                color += (closest_object->mat.color * light.color)
                       * glm::dot(normal, ray.direction);
            }else color = closest_object->mat.color * light.color;
        }
    }

    return color;
}

