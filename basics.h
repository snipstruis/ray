#pragma once

#include "glm/vec3.hpp"
#include "glm/glm.hpp"

// FIXME: looking for a better name for this file..
// Basic types, used in most places 

const int STARTING_TTL = 10; // probably should be configurable or dynamically calculated

struct Ray{
    Ray(glm::vec3 o, glm::vec3 d, int t):origin(o),direction(d),ttl(t){};
    glm::vec3 origin, direction;
    int ttl;
};

struct Color{
    Color() = default;
    Color(float rr,float gg,float bb):r(rr),g(gg),b(bb){};
    // TODO: adjust for color correction
    Color& operator*=(float f){
        r*=f; g*=f; b*=f;
        return *this;
    }
    float r,g,b;
};

struct Material{
    Material() = default;
    Material(Color c):color(c){};
    Color color;
};

