#pragma once

#include <vector>

#include "v3.h"

struct Ray{
    v3 origin;
    v3 direction;
};

struct Material{
    float r,g,b;
};

struct Object{
    v3 pos;
    Material mat;
    virtual bool intersects(Ray) = 0;
};

struct Plane:Object{
    v3 normal;
    virtual bool intersect(Ray r){
        return false;
    };
};

struct Sphere:Object{
    float radius;
    virtual bool intersect(Ray r){
        return false;
    };
};

struct PointLight{
    float intensity;
    float r,g,b;
    v3 pos;
};

struct Camera{
    v3 eye,screen_center,screen_corner;
};

struct Scene{
    Camera camera;
    std::vector<Object> objects;
    std::vector<PointLight> lights;
};

