#pragma once
#include "utils.h"
#include "basics.h"
#include "primitive.h"
#include "scene.h"
#include <cmath>
#include <vector>

Color diffuse(Ray const& ray,
              Primitives const& primitives,
              std::vector<PointLight> const& lights,
              Intersection const& hit,
              Material const& mat){
    Color color = Color(0,0,0);

    for(PointLight const& light: lights){
        glm::vec3 impact_to_light = light.pos - hit.impact;
        float light_distance = glm::length(impact_to_light);
        glm::vec3 light_direction = glm::normalize(impact_to_light);
        // TODO: when adding transparancy, fix the shadow epsilon,
        // it only works for one-sided surfaces
        Ray shadow_ray = Ray(hit.impact-(light_direction*1e-3f), 
                light_direction, 0);

        Intersection shadow_hit = findClosestIntersection(primitives, shadow_ray);

        if(shadow_hit.distance == INFINITY){
            color += mat.color 
                   * light.color * (1.f/light_distance)
                   * glm::dot(-hit.normal, ray.direction);
        }
    }


    return color;
}

Color trace(Ray const& ray,
            Primitives const& primitives,
            std::vector<PointLight> const& lights,
            Color const& alpha){
    if(ray.ttl<=0) return alpha;

    // hit check
    Intersection hit = findClosestIntersection(primitives,ray);
    if(hit.distance==INFINITY) return alpha;

    Material mat = primitives.materials[hit.mat];
    if(mat.checkered >= 0){
        int x = hit.impact.x-EPSILON;
        int y = hit.impact.y-EPSILON;
        int z = hit.impact.z-EPSILON;
        if((x&1)^(y&1)^(z&1)) 
            mat = primitives.materials[mat.checkered];
    }

    Color color = Color(0,0,0);

    if(mat.reflectiveness > 0.f){
        Ray r = Ray(hit.impact, glm::reflect(ray.direction, hit.normal), ray.ttl-1);
        color += mat.reflectiveness * trace(r,primitives,lights,alpha);
    }

    if(mat.diffuseness > 0.f){
        Color diffuse_color = diffuse(ray,primitives,lights,hit,mat);
        color += mat.diffuseness * diffuse_color;
    }

    /*
    if(mode & MAT_transparent){
        glm::vec3 refract_direction = glm::refract(ray.direction, hit.normal, 
                                        ray.refraction_index/mat.refraction_index);
        Ray refract_ray = Ray(hit.impact-(refract_direction*1e-3f),
                refract_direction, 
                ray.ttl-1, 
                mat.refraction_index);
        Color refract_color = trace(refract_ray, primitives, lights, alpha);
        color += refract_color;
    }
    */

    return color;
}

// main render starting loop
// assumes screenbuffer is big enough to handle the width*height pixels (per the camera)
inline void renderFrame(Scene& s, std::vector<Color>& screenBuffer){
    // draw pixels
    #pragma omp parallel for schedule(auto)
    for (int y = 0; y < s.camera.height; y++) {
        for (int x = 0; x < s.camera.width; x++) {
            Ray r = s.camera.makeRay(x, y);

            // go forth and render..
            int idx = (s.camera.height-y-1) * s.camera.width+ x;
            screenBuffer[idx] = trace(r,s.primitives,s.lights,Color(0,0,0));
        }
    }
}

