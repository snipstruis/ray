#pragma once 
#include "bvh_traverse.h"
#include "basics.h"
#include "primitive.h"
#include "scene.h"
#include "utils.h"

#include "glm/gtx/vector_query.hpp"

#include <cmath>
#include <chrono>
#include <vector>

// calculate point light colour output
inline Color calcLightOutput(PointLight const& light, 
                     float distance, 
                     Ray const& ray, 
                     FancyIntersection const& hit, 
                     Material const& mat,
                     glm::vec3 const& lightDir) {

    assert(isFinite(mat.diffuseColor));
    assert(isFinite(mat.specular_highlight));
    assert(isFinite(light.color));
    assert(glm::isNormalized(hit.normal, EPSILON));
    assert(glm::isNormalized(lightDir, EPSILON));

    float diff = glm::dot(hit.normal, lightDir);
    float falloff = 1.0f/distance;
    assert(std::isfinite(diff));
    assert(std::isfinite(falloff));

    Color ret = mat.diffuseColor * light.color * falloff * diff;
    assert(isFinite(ret));

    if(mat.specular_highlight!=BLACK){
        glm::vec3 refl = glm::reflect(lightDir,hit.normal);
        float dot = glm::dot(ray.direction,refl);
        if(dot>0.f){
            ret += powf(dot, mat.shininess) * mat.specular_highlight * light.color * falloff;
        } 
    }

    assert(isFinite(ret));
    return ret;
}

// calculate spot light colour output
inline Color calcLightOutput(SpotLight const& light, 
                     float distance, 
                     Ray const& ray, 
                     FancyIntersection const& hit, 
                     Material const& mat,
                     glm::vec3 const& lightDir) {

    float dot = fabs(glm::dot(-lightDir, light.pointDir));
    float outer = light.cosOuterAngle;
    if(dot < outer){ // outside outer cone
        return BLACK;
    }

    float inner = light.cosInnerAngle;
    Color lout = calcLightOutput(PointLight(light.pos,light.color), distance, ray, hit, mat, lightDir);

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
              Material const& mat,
              Params const& p){
    Color color = BLACK;

    for(auto const& light : lights){
        glm::vec3 impact_to_light = light.pos - hit.impact;
        float light_distance = glm::length(impact_to_light);
        glm::vec3 light_direction = glm::normalize(impact_to_light);

        Ray shadow_ray = Ray(hit.impact + (hit.normal*EPSILON), light_direction, ray.mat, ray.ttl-1);

        // does this shadow ray hit any geometry?
        bool shadow_hit = findAnyIntersectionBVH(bvh, primitives, shadow_ray, light_distance, p.traversalMode);

        if(!shadow_hit){
            color += calcLightOutput(light, light_distance, ray, hit, mat, light_direction);
        }
    }
    assert(isFinite(color));
    return color;
}

inline Color calcTotalDiffuse(Ray const& ray,
              BVH const& bvh,
              Primitives const& primitives,
              Lights const& lights,
              FancyIntersection const& hit,
              Material const& mat,
              Params const& p){

    Color color = Color(0,0,0);

    color += diffuse(ray, bvh, primitives, lights.pointLights, hit, mat, p);
    color += diffuse(ray, bvh, primitives, lights.spotLights, hit, mat, p);

    assert(isFinite(color));
    return color;
}

Color trace(Ray const& ray,
            BVH const& bvh,
            Primitives const& primitives,
            Lights const& lights,
            Color const& alpha,
            Params const& p){
    assert(isFinite(alpha));

    if(ray.ttl<=0) 
        return alpha;
    
    MiniIntersection hit = findClosestIntersectionBVH(bvh, primitives, ray, p.traversalMode);
    if(!hit.hit()) 
        return alpha;

    TrianglePos const& pos = primitives.pos[hit.triangle];
    TriangleExtra const& tri = primitives.extra[hit.triangle];
    FancyIntersection fancy = FancyIntersect(hit.distance, pos, tri, ray, p.smoothing);

    assert(glm::isNormalized(fancy.normal, EPSILON));

    Material mat = primitives.materials[fancy.mat];
    Material raymat = primitives.materials[ray.mat];

    if(mat.checkered >= 0){
        int x = (int)(fancy.impact.x - EPSILON);
        int y = (int)(fancy.impact.y - EPSILON);
        int z = (int)(fancy.impact.z - EPSILON);
        if((x&1)^(y&1)^(z&1)) 
            mat = primitives.materials[mat.checkered];
    }
	
    Color color = BLACK;

    // shadows and lighting
    if(mat.diffuseColor!=BLACK){
        color += calcTotalDiffuse(ray, bvh, primitives, lights, fancy, mat, p);
    }

    assert(isFinite(color));

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
        Ray refract_ray = Ray(fancy.impact-(fancy.normal*EPSILON),
                refract_direction, 
                //FIXME: exiting a primitive will set the material to air
                fancy.internal ? MATERIAL_AIR : fancy.mat, 
                ray.ttl-1);
        color += transparency * trace(refract_ray, bvh, primitives, lights, alpha, p);
    }
    
    // reflection (mirror)
    if(reflectiveness!=BLACK){
        Ray r = Ray(fancy.impact+fancy.normal*EPSILON,
                    glm::reflect(ray.direction, fancy.normal),
                    ray.mat,
                    ray.ttl-1);
        color += reflectiveness * trace(r, bvh, primitives, lights, alpha, p);
    }

    // absorption (Beer's law)
    if(ray.mat!=MATERIAL_AIR){
        color.r *= expf( -raymat.diffuseColor.r * hit.distance);
        color.g *= expf( -raymat.diffuseColor.g * hit.distance);
        color.b *= expf( -raymat.diffuseColor.b * hit.distance);
    }
    return color;
}

