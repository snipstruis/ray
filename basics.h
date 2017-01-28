#pragma once

#include "utils.h"

#include "glm/vec3.hpp"
#include "glm/glm.hpp"

#include <vector>

// Basic types, used in most places 


// indirect mapping into the triangle array(s). used in bvh (et al)
typedef std::vector<unsigned int> TriangleMapping;

const int STARTING_TTL = 10; // probably should be configurable or dynamically calculated

struct Ray{
    Ray(glm::vec3 o, glm::vec3 d, int m, int t)
        :origin(o), direction(d), mat(m), ttl(t) {};

    glm::vec3 origin, direction;
    int mat;
    int ttl;
};

typedef glm::vec3 Color;
typedef std::vector<Color> ScreenBuffer;

inline bool isFinite(Color const& c){
    return std::isfinite(c.r) && std::isfinite(c.g) && std::isfinite(c.b);
}

inline bool valLegal(float val) {
    return std::isfinite(val) && val >= 0.0f && val <= 1.0f;
}

// Check col is legal for final output (ie [0.0-1.0])
inline bool isLegal(Color const& c) {
    return valLegal(c.r) && valLegal(c.g) && valLegal(c.b);
}

inline Color colorClamp(Color c, float min=0.f, float max=1.f) {
    return Color(clamp(c.r, min, max), clamp(c.g, min, max), clamp(c.b, min, max));
}

inline Color colorCorrect(Color const& c){
    return Color(sqrtf(c.r),sqrtf(c.g),sqrtf(c.b));
}

#define BLACK Color(0,0,0)

