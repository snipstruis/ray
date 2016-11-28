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

        Ray shadow_ray = Ray(hit.impact+(hit.normal*1e-4f), 
                light_direction, ray.mat, ray.ttl-1);

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

    Intersection hit = findClosestIntersection(primitives,ray);
    if(hit.distance==INFINITY) return alpha;

    Material mat = primitives.materials[hit.mat];
    Material raymat = primitives.materials[ray.mat];
    if(mat.checkered >= 0){
        int x = hit.impact.x-1e-6f;
        int y = hit.impact.y-1e-6f;
        int z = hit.impact.z-1e-6f;
        if((x&1)^(y&1)^(z&1)) 
            mat = primitives.materials[mat.checkered];
    }

    Color color = Color(0,0,0);

    // shadows and lighting
    if(mat.diffuseness > 0.f){
        color += mat.diffuseness * diffuse(ray,primitives,lights,hit,mat);
    }

    // angle-depenent transparancy (for dielectric materials)
    float reflectiveness = mat.reflectiveness;
    float transparency   = mat.transparency;
    if(mat.transparency > 0.f){
        float n1 = raymat.refraction_index;
        float n2 = mat.refraction_index;
        float r0 = (n1-n2)/(n1+n2); r0*=r0;
        float pow5 = 1.f-glm::dot(hit.normal,-ray.direction);
        float fr = r0+(1.f-r0)*pow5*pow5*pow5*pow5*pow5;
        reflectiveness += transparency*fr;
        transparency   -= transparency*fr;
    }
       
    // transparency (refraction)
    if(transparency>0.f){
        glm::vec3 refract_direction = 
            glm::refract(ray.direction, hit.normal, 
                    raymat.refraction_index/(hit.internal?1.f:mat.refraction_index));
        Ray refract_ray = Ray(hit.impact-(hit.normal*1e-4f),
                refract_direction, 
                //FIXME: exiting a primitive will set the material to air
                hit.internal?0:hit.mat, 
                ray.ttl-1);
        color += transparency * trace(refract_ray, primitives, lights, alpha);
    }
    
    // reflection (mirror)
    if(reflectiveness > 0.f){
        Ray r = Ray(hit.impact+hit.normal*1e-4f,
                    glm::reflect(ray.direction, hit.normal),
                    ray.mat,
                    ray.ttl-1);
        color += reflectiveness * trace(r,primitives,lights,alpha);
    }

    // absorption (Beer's law)
    if(ray.mat>0){
        color.r *= expf( -raymat.color.r * hit.distance);
        color.g *= expf( -raymat.color.g * hit.distance);
        color.b *= expf( -raymat.color.b * hit.distance);
    }
    return color;
}

#include<chrono>

enum class Visualisation {
    None,
    Microseconds,
};

// main render starting loop
// assumes screenbuffer is big enough to handle the width*height pixels (per the camera)
inline void renderFrame(Scene& s, std::vector<Color>& screenBuffer, Visualisation vis){
    // draw pixels
    #pragma omp parallel for schedule(auto)
    for (int y = 0; y < s.camera.height; y++) {
        for (int x = 0; x < s.camera.width; x++) {
            auto start = std::chrono::high_resolution_clock::now();
            Ray r = s.camera.makeRay(x, y);

            // go forth and render..
            int idx = (s.camera.height-y-1) * s.camera.width+ x;

            screenBuffer[idx] = trace(r,s.primitives,s.lights,Color(0,0,0));
            if(vis==Visualisation::Microseconds){
                auto end = std::chrono::high_resolution_clock::now();
                auto frametime = 
                    std::chrono::duration_cast<std::chrono::duration<float,std::micro>>(end-start).count();
                screenBuffer[idx].r = frametime;
            }
        }
    }
}

