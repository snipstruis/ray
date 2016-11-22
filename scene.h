#pragma once

#include "glm/vec3.hpp"
#include <vector>

struct Ray{
    glm::vec3 origin, direction;
    int ttl;
};

struct Color{
    float r,g,b;
};

struct Material{
    Color color;
    Color recurse(Ray ray, glm::vec3 normal);
};

#include "primitive.h"

struct PointLight{
    float r,g,b;
    glm::vec3 pos;
};

#include "camera.h"

struct Object {
};

struct Scene{
    Camera camera;
    std::vector<Object> objects;
    std::vector<PointLight> lights;
};

