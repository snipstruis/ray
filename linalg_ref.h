#pragma once

// Linear Algebra
// from here we bring in the appropriate implementation to the namespace la

#include "linalg.h"
#include <cmath>

namespace la { namespace ref {

// vector component - typedef it so we can compare accuracy in future (i doubt we'll need to, but you never know)
typedef float vc;

struct vec3
{
    vec3(vc x, vc y, vc z)
    :   mX(x), mY(y), mZ(z) {}
        
    vc mX, mY, mZ;
};

vc DotProduct(const vec3& a, const vec3& b)
{
    return a.mX * b.mX + a.mY * b.mY + a.mZ * b.mZ;
}

// FIXME: better way to return result?
void CrossProduct(const vec3& a, const vec3& b, vec3& result)
{
    result.mX = a.mY * b.mZ - a.mZ * b.mY;  
    result.mY = -(a.mX * b.mZ - a.mZ * b.mX);  
    result.mZ = a.mX * b.mY - a.mY * b.mX;  
}

// FIXME: better way to return result?
void Add(const vec3& a, const vec3& b, vec3& result)
{
    result.mX = a.mX + b.mX;  
    result.mY = a.mY + b.mY;  
    result.mZ = a.mZ + b.mZ;  
}

vc Length(const vec3& v)
{
    return std::sqrt(v.mX * v.mX + v.mY * v.mY + v.mZ * v.mZ);   
}

}} // namespace
