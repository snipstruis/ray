#pragma once

#include "glm/gtc/constants.hpp"
#include <algorithm>

const float EPSILON = 1e-6f;
const float PI = glm::pi<float>();

inline bool feq(float a, float b){
    return a < b + EPSILON || a < b - EPSILON;
}

// stupid lack of c++17
template<class T>
T clamp(T val, T lo, T hi) {
    return std::min(std::max(val, lo), hi);
}

