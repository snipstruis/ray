#pragma once

#include "bvh_traverse.hpp"
#include "color.hpp"
#include "scene.hpp"
#include "triangle.hpp"
#include "utils.hpp"

#include "glm/gtx/vector_query.hpp"

#include <cmath>
#include <vector>

Color pathTrace(
        const Ray& ray,
        const BVH& bvh,
        const Scene& scene,
        const Params& p,
        bool prevMirror);

static Rng rng = Rng(1337);

// thanks Jacco!
glm::vec3 diffuseDirectionCos(glm::vec3 const& norm){
    // random point on unit disk
    float rr = rng.floatRange(0.f,1.f);
    float r  = sqrt(rr);
    float th = rng.floatRange(0,2.f*PI);
    float x  = r*cosf(th);
    float y  = r*sinf(th);
    // project it on hemisphere
    glm::vec3 v = glm::vec3(x,y,sqrt(1.f-rr));
    // transform to tangent space
    glm::vec3 diff= glm::cross(glm::vec3(0,0,1),norm);
    return glm::rotate(v, glm::length(diff), glm::normalize(diff));
}

glm::vec3 diffuseDirectionUni(glm::vec3 const& norm){
    float theta = rng.floatRange( 0.0f, 2.0f * PI);
    float r = sqrt( rng.floatRange( 0.0f, 1.0f ) );
    float z = sqrt( 1.0f - r*r ) * (rng.asUint()&1? 1.f:-1.f);
    return glm::vec3( r * cos(theta), r * sin(theta), z );
}

inline glm::vec3 random_point_on_triangle(TrianglePos const& t){
    // first produce random point on unit square
    float a = rng.floatRange(0.f,1.f);
    float b = rng.floatRange(0.f,1.f);
    // if it's going to produce a point outside the lower left triangle, invert coords
    if(a+b>1.f){ a=1.f-a; b=1.f-b; }
    // transform the (0,0) (0,1) (1,0) triangle to the triangle shape
    return t.v[0] + a*(t.v[1]-t.v[0]) + b*(t.v[2]-t.v[0]);
}

Color directIllumination(Scene const& scene, FancyIntersection const& fancy, 
                         BVH const& bvh, Params const& p, Material const& mat){
    // picking random point on light
    auto const& light_indices = scene.primitives.light_indices;
    int const  random_index = light_indices[rng.intRange(0,light_indices.size()-1)];
    TrianglePos const& random_triangle = scene.primitives.pos[random_index];
    glm::vec3 l = random_point_on_triangle(random_triangle) - fancy.impact;
    float dist = glm::length(l);
    l /= dist;

    // culling
    glm::vec3 lightNormal = scene.primitives.extra[random_index].n[0]; //good enough?
    float cos_o = glm::dot(-l, lightNormal);
    if(cos_o<=0.f) return BLACK;
    float cos_i = glm::dot( l, fancy.normal);
    if(cos_i<=0.f) return BLACK;

    // light not behind face, trace shadow ray
    Ray shadowray = Ray(fancy.impact+EPSILON*l, l, 1);
    if(findAnyIntersectionBVH(bvh, scene.primitives, shadowray, 
                              dist-2*EPSILON, p.traversalMode)) return BLACK;

    // calculate transport
    auto lightmat = scene.primitives.materials[scene.primitives.extra[random_index].mat];
    glm::vec3 BRDF = mat.diffuse() * INVPI;
    float solidAngle = (cos_o*random_triangle.area()) / (dist*dist);
    return BRDF * (float)light_indices.size() * lightmat.emissive() * solidAngle * cos_i;
}

Color indirectIllumination(Scene const& scene, FancyIntersection const& fancy, 
        BVH const& bvh, Params const& p, Material const& mat, Ray ray, bool prevMirror){
    // terminate if we hit a light source 
    if (mat.isEmissive()) {
        if(prevMirror) // lights should look bright
            return mat.emissive();
        else return BLACK; // but not count towards indirect illumination
    }

    float reflect_chance = (mat.reflective().r+mat.reflective().g+mat.reflective().b)/3.f;
    
    // handle mirrors
    if(mat.isReflective()){
        glm::vec3 refl = glm::reflect(ray.direction, fancy.normal);
        Ray reflray = Ray(fancy.impact+fancy.normal*EPSILON, refl, ray.ttl-1);
        return pathTrace(reflray, bvh, scene, p, prevMirror);
    }

    // rest: handle diffuse

    prevMirror=false;

    // continue in random direction
    glm::vec3 direction = diffuseDirectionCos(fancy.normal);
    Ray newray(fancy.impact + EPSILON*fancy.normal,
               direction,
               ray.ttl-1);

    glm::vec3 BRDF = mat.diffuse() * INVPI;
    float PDF = glm::dot(fancy.normal,direction)*INVPI;
    Color ii = glm::dot(fancy.normal, direction) * pathTrace(newray, bvh, scene, p, prevMirror) / PDF;
    return BRDF * ii;
}
        
Color pathTrace(
        const Ray& ray,
        const BVH& bvh,
        const Scene& scene,
        const Params& p,
        bool prevMirror = true) {
    if(ray.ttl == 0) return BLACK;
    
    const MiniIntersection mini = 
        findClosestIntersectionBVH(bvh, scene.primitives, ray, p.traversalMode);

    // terminate if ray left the scene
    if(!mini.hit()) return BLACK;

    const FancyIntersection fancy = 
        FancyIntersect(mini, scene.primitives, ray, p.smoothing);
    const Material& mat = scene.primitives.materials[fancy.mat];

    // direct illumination
    Color di = directIllumination(scene, fancy, bvh, p, mat);
    Color ii = indirectIllumination(scene, fancy, bvh, p, mat, ray, true);
    return di + ii;
}

