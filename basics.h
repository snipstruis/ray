#pragma once

#include "utils.h"

#include "glm/vec3.hpp"
#include "glm/glm.hpp"

#include <vector>

// Basic types, used in most places 

struct Color;
typedef std::vector<Color> ScreenBuffer;

// indirect mapping into the triangle array(s). used in bvh (et al)
typedef std::vector<std::uint32_t> TriangleMapping;

const int STARTING_TTL = 1; // probably should be configurable or dynamically calculated

struct Ray{
    Ray(glm::vec3 o, glm::vec3 d, int m, int t)
        :origin(o), direction(d), mat(m), ttl(t) {};

    glm::vec3 origin, direction;
    int mat;
    int ttl;
};

