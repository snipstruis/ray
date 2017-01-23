#pragma once

#include "bvh_traverse.h"
#include "scene.h"
#include "primitive.h"
#include "utils.h"

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
    assert(light_indices.size()>0);
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
    Ray shadowray = Ray(fancy.impact+EPSILON*l, l, 0, 1);
    if(findAnyIntersectionBVH(bvh, scene.primitives, shadowray, 
                              dist-2*EPSILON, p.traversalMode)) return BLACK;

    // calculate transport
    auto lightmat = scene.primitives.materials[scene.primitives.extra[random_index].mat];
    glm::vec3 BRDF = mat.diffuseColor * INVPI;
    float solidAngle = (cos_o*random_triangle.area()) / (dist*dist);
    return BRDF * (float)light_indices.size() * lightmat.emissive * solidAngle * cos_i;
}

Color indirectIllumination(Scene const& scene, FancyIntersection const& fancy, 
        BVH const& bvh, Params const& p, Material const& mat, 
        Ray ray, Material const& raymat, bool prevMirror){

    // terminate if we hit a light source 
    if (mat.emissive!=BLACK) {
        if(prevMirror) // lights should look bright
            return mat.emissive;
        else return BLACK; // but not count towards indirect illumination
    }

    Color reflectiveness = mat.reflectiveness;
    float transparency   = mat.transparency;

    // angle-depenent transparancy (for dielectric materials)
    if(mat.transparency > 0.f){
        float n1 = raymat.refraction_index;
        float n2 = mat.refraction_index;
        float r0 = (n1-n2)/(n1+n2); r0*=r0;
        float pow5 = 1.f-glm::dot(fancy.normal, -ray.direction);
        float fr = r0+(1.f-r0)*pow5*pow5*pow5*pow5*pow5;
        reflectiveness += transparency*fr;
        transparency   -= transparency*fr;
    }

    // transparancy
    if(transparency>rng.floatRange(0,1)){
        glm::vec3 refract_direction = 
            glm::refract(ray.direction, fancy.normal, 
                    raymat.refraction_index/(fancy.internal?1.f:mat.refraction_index));
        Ray refract_ray = Ray(fancy.impact-(fancy.normal*1e-4f),
                refract_direction, 
                //FIXME: exiting a primitive will set the material to air
                fancy.internal ? MATERIAL_AIR : fancy.mat, 
                ray.ttl-1);
        return pathTrace(refract_ray, bvh, scene, p, prevMirror);
    }


    // handle mirrors
    if(reflectiveness.r+reflectiveness.g+reflectiveness.b > rng.floatRange(0,3)){
        glm::vec3 refl = glm::reflect(ray.direction, fancy.normal);
        Ray reflray = Ray(fancy.impact+fancy.normal*EPSILON, refl, ray.mat, ray.ttl-1);
        return pathTrace(reflray, bvh, scene, p, prevMirror);
    }


    // rest: handle diffuse

    prevMirror=false;

    // continue in random direction
    glm::vec3 direction = diffuseDirectionCos(fancy.normal);
    Ray newray(fancy.impact + EPSILON*fancy.normal,
               direction,
               ray.mat,
               ray.ttl-1);

    glm::vec3 BRDF = mat.diffuseColor * INVPI;
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
        FancyIntersect(mini.distance, scene.primitives.pos[mini.triangle], 
                                      scene.primitives.extra[mini.triangle], 
                                      ray, p.smoothing);
    const Material& mat = scene.primitives.materials[fancy.mat];
    const Material& raymat = scene.primitives.materials[ray.mat];

    // direct illumination
    Color di = directIllumination(scene, fancy, bvh, p, mat);
    Color ii = indirectIllumination(scene, fancy, bvh, p, mat, ray, raymat, true);
    return di + ii;
}

