#pragma once

#include "bvh_traverse.h"
#include "color.h"
#include "basics.h"
#include "primitive.h"
#include "scene.h"
#include "utils.h"

#include <cmath>
#include <chrono>
#include <vector>

// calculate point light colour output
inline Color calcLightOutput(PointLight const& light, 
                     float distance, 
                     Ray const& ray, 
                     FancyIntersection const& hit, 
                     Material const& mat,
                     glm::vec3 light_dir) {
    float diff = glm::dot(hit.normal, light_dir);
    float falloff = 1.f/distance;
    Color ret = mat.diffuseColor * light.color * falloff * diff;

    if(!mat.specular_highlight.isBlack()){
        glm::vec3 refl = glm::reflect(light_dir,hit.normal);
        float dot = glm::dot(ray.direction,refl);
        if(dot>0.f){
            ret += powf(dot, mat.shininess) * mat.specular_highlight * light.color * falloff;
        } 
    }
    return ret;
}

// calculate spot light colour output
inline Color calcLightOutput(SpotLight const& light, 
                     float distance, 
                     Ray const& ray, 
                     FancyIntersection const& hit, 
                     Material const& mat,
                     glm::vec3 light_dir) {
    float dot = fabs(glm::dot(-light_dir, light.pointDir));
    float outer = light.cosOuterAngle;
    if(dot < outer){ // outside outer cone
        return Color(0,0,0);
    }
    float inner = light.cosInnerAngle;
    Color lout = calcLightOutput( PointLight(light.pos,light.color),
                                  distance,ray,hit,mat,light_dir);
    if(dot > inner){ // inside inner cone
        return lout;
    }else{ // between inner and outer cones
        float ratio = 1.f - (dot-inner)/(outer-inner);
        return ratio*lout;
    }
}

template <class LightsType>
inline Color diffuse(Ray const& ray,
              BVH const& bvh,
              Primitives const& primitives,
              LightsType const& lights,
              FancyIntersection const& hit,
              Material const& mat){
    Color color = Color(0,0,0);

    for(auto const& light : lights){
        glm::vec3 impact_to_light = light.pos - hit.impact;
        float light_distance = glm::length(impact_to_light);
        glm::vec3 light_direction = glm::normalize(impact_to_light);

        Ray shadow_ray = Ray(hit.impact + (hit.normal*1e-4f), light_direction, ray.mat, ray.ttl-1);

        // does this shadow ray hit any geometry?
        bool shadow_hit = findAnyIntersectionBVH(bvh, primitives, shadow_ray, light_distance);

        if(!shadow_hit){
            color += calcLightOutput(light, light_distance, ray, hit, mat, light_direction);
        }
    }
    return color;
}

inline Color calcTotalDiffuse(Ray const& ray,
              BVH const& bvh,
              Primitives const& primitives,
              Lights const& lights,
              FancyIntersection const& hit,
              Material const& mat){
    Color color = Color(0,0,0);

    color += diffuse(ray, bvh, primitives, lights.pointLights, hit, mat);
    color += diffuse(ray, bvh, primitives, lights.spotLights, hit, mat);

    return color;
}

Color trace(Ray const& ray,
            BVH const& bvh,
            Primitives const& primitives,
            Lights const& lights,
            Color const& alpha){

    if(ray.ttl<=0) return alpha;

    MiniIntersection hit = findClosestIntersectionBVH(bvh, primitives, ray);
    if(hit.distance==INFINITY) 
        return alpha;

    Triangle const& tri = primitives.triangles[hit.triangle];
    FancyIntersection fancy = FancyIntersect(hit.distance, tri, ray);
    Material mat = primitives.materials[fancy.mat];
    Material raymat = primitives.materials[ray.mat];

    if(mat.checkered >= 0){
        int x = (int)(fancy.impact.x - EPSILON);
        int y = (int)(fancy.impact.y - EPSILON);
        int z = (int)(fancy.impact.z - EPSILON);
        if((x&1)^(y&1)^(z&1)) 
            mat = primitives.materials[mat.checkered];
    }
	
    Color color = Color(0, 0, 0);

    // shadows and lighting
    if(!mat.diffuseColor.isBlack()){
        color += calcTotalDiffuse(ray, bvh, primitives, lights, fancy, mat);
    }

    // angle-depenent transparancy (for dielectric materials)
    Color reflectiveness = mat.reflectiveness;
    float transparency = mat.transparency;

    if(mat.transparency > 0.f){
        float n1 = raymat.refraction_index;
        float n2 = mat.refraction_index;
        float r0 = (n1-n2)/(n1+n2); r0*=r0;
        float pow5 = 1.f-glm::dot(fancy.normal, -ray.direction);
        float fr = r0+(1.f-r0)*pow5*pow5*pow5*pow5*pow5;
        reflectiveness += transparency*fr;
        transparency   -= transparency*fr;
    }
       
    // transparency (refraction)
    if(transparency>0.f){
        glm::vec3 refract_direction = 
            glm::refract(ray.direction, fancy.normal, 
                    raymat.refraction_index/(fancy.internal?1.f:mat.refraction_index));
        Ray refract_ray = Ray(fancy.impact-(fancy.normal*1e-4f),
                refract_direction, 
                //FIXME: exiting a primitive will set the material to air
                fancy.internal ? MATERIAL_AIR : fancy.mat, 
                ray.ttl-1);
        color += transparency * trace(refract_ray, bvh, primitives, lights, alpha);
    }
    
    // reflection (mirror)
    if(!reflectiveness.isBlack()){
        Ray r = Ray(fancy.impact+fancy.normal*1e-4f,
                    glm::reflect(ray.direction, fancy.normal),
                    ray.mat,
                    ray.ttl-1);
        color += reflectiveness * trace(r, bvh, primitives,lights,alpha);
    }

    // absorption (Beer's law)
    if(ray.mat!=MATERIAL_AIR){
        color.r *= expf( -raymat.diffuseColor.r * hit.distance);
        color.g *= expf( -raymat.diffuseColor.g * hit.distance);
        color.b *= expf( -raymat.diffuseColor.b * hit.distance);
    }
    return color;
}


enum class Mode {
    Default,
    Microseconds,
    Normal,
};

// main render starting loop
// assumes screenbuffer is big enough to handle the width*height pixels (per the camera)
inline void renderFrame(Scene& s, BVH& bvh, std::vector<Color>& screenBuffer, Mode mode){
    // draw pixels
    int const width  = s.camera.width;
    int const height = s.camera.height;
    switch(mode){
    case Mode::Default:
        #pragma omp parallel for schedule(auto) collapse(2)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Ray r = s.camera.makeRay(x, y);
                int idx = (height-y-1) * width+ x;
                auto col = trace(r, bvh, s.primitives, s.lights, Color(0,0,0));
                screenBuffer[idx] = ColorClamp(col, 0.0f, 1.0f);
            }
        }
        break;
    case Mode::Microseconds:
        #pragma omp parallel for schedule(auto) collapse(2)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                auto start = std::chrono::high_resolution_clock::now();
                Ray r = s.camera.makeRay(x, y);
                int idx = (height-y-1) * width+ x;
                screenBuffer[idx] = trace(r, bvh, s.primitives, s.lights, Color(0,0,0));
                auto end = std::chrono::high_resolution_clock::now();
                auto frametime = 
                    std::chrono::duration_cast<std::chrono::duration<float,std::micro>>(end-start).count();
                screenBuffer[idx].r = frametime;
            }
        }
        break;
    case Mode::Normal:
#if 0
        #pragma omp parallel for schedule(auto) collapse(2)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Ray r = s.camera.makeRay(x,y);
                int idx = (height-y-1) * width+ x;
                auto hit = findClosestIntersection(s.primitives,r);
                screenBuffer[idx] = Color(hit.normal.x, hit.normal.y, hit.normal.z);
            }
        }
#endif
        break;
    }
}

