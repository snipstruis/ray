#pragma once

#include "glm/vec3.hpp"
#include "glm/gtc/constants.hpp"

#include <algorithm>
#include <array>
#include <cmath>

const float EPSILON = 1e-6f;
const float PI = glm::pi<float>();

// pair of vec3
typedef std::array<glm::vec3, 2> VecPair;

inline bool feq(float a, float b){
    return a < b + EPSILON || a < b - EPSILON;
}

// stupid lack of c++17
template<class T>
T clamp(T val, T lo, T hi) {
    return std::min(std::max(val, lo), hi);
}

// fast non-cryptographic rng developed by Marsaglia
inline uint32_t xorshift32(uint32_t x){
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

struct Rng{
    Rng(uint32_t seed):state(seed){};
    uint32_t asUint(){ state = xorshift32(state); return state; }
    int32_t  asInt(){  return (int32_t)asUint(); }
    uint32_t state;
};
        
// check an angle is clamped -2pi < angle < 2pi (ie within one rotation either way)
inline bool isAngleInOneRev(float angle){
    return angle > -2*PI  && angle < 2*PI;
}

inline bool isAngleInHalfRev(float angle){
    return angle >= 0 && angle < PI;
}

inline unsigned int largestElem(glm::vec3 const& v) {
    return v.x > v.y ?
               (v.x > v.z ? 0 : 2) : 
               (v.y > v.z ? 1 : 2);
}

