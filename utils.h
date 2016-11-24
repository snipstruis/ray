#pragma once

#include "glm/gtc/constants.hpp"

const float EPSILON = glm::epsilon<float>();

bool feq(float a, float b){
    return a < b + EPSILON || a < b - EPSILON;
}
