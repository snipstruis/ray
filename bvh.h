#pragma once

#include "glm/vec3.hpp"
#include <cl.hpp>

struct AABB {
    glm::vec3 a,b;
};


struct BVHNode{
    AABB bounds;
    int leftFirst;
    int count;
};

// check sizes are as expected 
static_assert(sizeof(BVHNode) == 32, "BVHNode size");

