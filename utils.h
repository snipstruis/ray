#pragma once

#include "glm/gtc/constants.hpp"
#include <algorithm>
#include <cmath>

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

// check an angle is clamped -2pi < angle < 2pi (ie within one rotation either way)
inline bool isAngleInOneRev(float angle){
    return angle > -2*PI  && angle < 2*PI;
}

inline bool isAngleInHalfRev(float angle){
    return angle >= 0 && angle < PI;
}

