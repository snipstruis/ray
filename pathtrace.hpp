#pragma once

#include "bvh_traverse.hpp"
#include "color.hpp"
#include "scene.hpp"
#include "triangle.hpp"
#include "utils.hpp"

#include "glm/gtx/vector_query.hpp"

#include <cmath>
#include <vector>

static Rng rng = Rng(1337);

glm::vec3 diffuseDirection(const glm::vec3 norm){
    glm::vec3 v = glm::normalize(glm::vec3(rng.asInt(), rng.asInt(), rng.asInt()));
    if(glm::dot(v,norm)<0.f) return -v;
    else return v;
}

Color pathTrace(
        const Ray& ray,
        const BVH& bvh,
        const Scene& scene,
        const Params& p) {
    if(ray.ttl == 0) 
        return BLACK;
    
    const MiniIntersection mini = findClosestIntersectionBVH(bvh, scene.primitives, ray, p.traversalMode);

    // terminate if ray left the scene
    if(!mini.hit()) 
        return BLACK;

    const FancyIntersection fancy = FancyIntersect(mini, scene.primitives, ray, p.smoothing);
    const Material& mat = scene.primitives.materials[fancy.mat];

    // terminate if we hit a light source 
    if (mat.isEmissive()) 
        return mat.emissive(); // surface interaction

    // continue in random direction
    Ray newray(fancy.impact + EPSILON*fancy.normal,
               diffuseDirection(fancy.normal),
               ray.ttl-1);

    return mat.diffuse()*pathTrace(newray,bvh,scene,p);
}

