#pragma once

#include "glm/vec3.hpp"
#include "glm/gtc/constants.hpp"

#include <algorithm>
#include <array>
#include <cmath>

float const EPSILON = 1e-6f;
float const PI    = glm::pi<float>();
float const INVPI = 1/PI;

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
    uint32_t state;
    uint32_t asUint(){ state = xorshift32(state); return state; }
    int32_t  asInt(){ return (int32_t)asUint(); }
    int32_t  intRange(int min, int max){ //inclusive
        assert(min<max);
        // yes it is not strictly uniform, but it's good enough
        // the alternative is looping when the value is within
        // the last division of the end
        int32_t ret = asUint()%(max-min+1)+min;
        assert(min<=ret);
        assert(ret<=max);
        return ret;
    }
    float    floatRange(float min, float max){ //inclusive
        assert(min<max);
        // same non-uniformity here, don't worry about it
        float ret = ((float)asUint()/0xffFFffFF) * (max-min) + min;
        assert(min<=ret);
        assert(ret<=max);
        return ret;
    }
    bool      asBool(){return asUint()&1;}
    glm::vec3 asUnitVector(){
       float theta = floatRange(0.f,2.f*PI);
       float r = sqrt(floatRange(0.f,1.f));
       float z = sqrt(1.f-r*r);// * (asBool()?-1.f:1.f);
       return glm::vec3(r*cos(theta), r*sin(theta), z);
    }
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

