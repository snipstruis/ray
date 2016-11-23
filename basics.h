#pragma once

#include "glm/vec3.hpp"
#include "glm/glm.hpp"

// FIXME: looking for a better name for this file..
// Basic types, used in most places 

struct Ray{
    Ray() = default;
    Ray(glm::vec3 o, glm::vec3 d, int t=10):origin(o),direction(d),ttl(t){};
    glm::vec3 origin, direction;
    int ttl;
};

struct Color{
    float r,g,b;
};

struct Material{
    Color color;
};

