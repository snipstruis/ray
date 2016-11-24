#pragma once

#include "glm/gtc/constants.hpp"

const float EPSILON = 1e-6f;

bool feq(float a, float b){
    return a < b + EPSILON || a < b - EPSILON;
}
