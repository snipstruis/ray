#pragma once
#include "v3.h"

struct Ray{
    v3 origin;
    v3 direction;
    int ttl;
};

struct Color{
    float r,g,b;
};

struct Material{
    Color color;
    Color recurse(Ray ray, v3 normal);
};

#include "primitive.h"

struct PointLight{
    float r,g,b;
    v3 pos;
};

#include "camera.h"

#include<vector>
struct Scene{
    Camera camera;
    std::vector<Object> objects;
    std::vector<PointLight> lights;
};

