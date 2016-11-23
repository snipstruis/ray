#pragma once

#include "glm/vec3.hpp"

// FIXME: looking for a better name for this file..
// Basic types, used in most places 

struct Ray{
    // glm::vec has a default constructor, so just ensure ttl is defined
    Ray() : ttl(0) {} ;

    Ray(const glm::vec3& origin, const glm::vec3& direction, int ttl) 
        : origin(origin), direction(direction), ttl(ttl) {} ;

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

