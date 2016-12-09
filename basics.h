#pragma once

#include "utils.h"

#include "glm/vec3.hpp"
#include "glm/glm.hpp"

// FIXME: looking for a better name for this file..
// Basic types, used in most places 

const int STARTING_TTL = 10; // probably should be configurable or dynamically calculated

struct Ray{
    Ray(glm::vec3 o, glm::vec3 d, int m, int t)
        :origin(o), direction(d), mat(m), ttl(t) {};

    glm::vec3 origin, direction;
    int mat;
    int ttl;
};

