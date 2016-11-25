#pragma once

#include "glm/gtc/constants.hpp"

const float EPSILON = 1e-6f;
const float PI = glm::pi<float>();

inline bool feq(float a, float b){
    return a < b + EPSILON || a < b - EPSILON;
}

