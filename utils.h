#pragma once

#include "glm/gtc/constants.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

const float EPSILON = 1e-6f;
const float PI = glm::pi<float>();

inline bool feq(float a, float b){
    return a < b + EPSILON || a < b - EPSILON;
}

struct Timer {
    Timer() : last(std::chrono::high_resolution_clock::now()) {}

    // returns seconds since last sample
    float sample() {
        auto now = std::chrono::high_resolution_clock::now();
        // for some reason, std::seconds doesn't seem to be defined on my sys
        float diff = 
            std::chrono::duration_cast<std::chrono::duration<float,std::milli>>(now - last).count() / 1000.0f;
        last = now;
        return diff;
    }
    
    std::chrono::time_point<std::chrono::high_resolution_clock> last;
};

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

