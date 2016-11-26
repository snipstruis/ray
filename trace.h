#pragma once
#include "utils.h"
#include "basics.h"
#include "primitive.h"
#include "scene.h"
#include <cmath>

Color trace(Ray ray,
            Primitives const& primitives,
            std::vector<PointLight> const& lights,
            Color const& alpha){
    if(ray.ttl<=0) return alpha;

    // hit check
    Intersection hit = findClosestIntersection(primitives,ray);
    if(hit.distance==INFINITY) return alpha;
    Material mat = primitives.materials[hit.mat];
    unsigned const mode = mat.properties;

    glm::vec3 impact = hit.impact;
    glm::vec3 normal = hit.normal;

    Color color = Color(0,0,0);

    if(mode & MAT_lit) color += mat.color;

    // shadow
    for(PointLight const& light: lights){
        glm::vec3 impact_to_light = light.pos - impact;
        float light_distance = glm::length(impact_to_light);
        glm::vec3 light_direction = glm::normalize(impact_to_light);
        // TODO: when adding transparancy, fix the shadow epsilon,
        // it only works for one-sided surfaces
        Ray shadow_ray = Ray(impact-(light_direction*1e-3f), 
                light_direction, 0);

        Intersection shadow_hit = findClosestIntersection(primitives, shadow_ray);

        if(shadow_hit.distance == INFINITY){
            Color addition = mat.color * light.color * (1.f/light_distance);
            if(mode & MAT_diffuse){
                addition *= glm::dot(-normal, ray.direction);
            }
            if(mode & MAT_specular){
                Ray r = Ray(impact,glm::reflect(ray.direction,normal),ray.ttl-1);
                addition = mat.specularity * trace(r,primitives,lights,alpha)
                         + (1.f-mat.specularity) * addition;
            }
            color += addition;
        }
    }

    if(mode & MAT_checkered){
        int x = impact.x-EPSILON;
        int y = impact.y-EPSILON;
        int z = impact.z-EPSILON;
        if((x&1)^(y&1)^(z&1)) color *= 0.8f;
    }

    return color;
}

