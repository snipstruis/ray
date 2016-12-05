#pragma once

#include "glm/vec3.hpp"
#include "glm/glm.hpp"

// FIXME: looking for a better name for this file..
// Basic types, used in most places 

const int STARTING_TTL = 10; // probably should be configurable or dynamically calculated

struct Ray{
    Ray(glm::vec3 o, glm::vec3 d, int m, int t)
        :origin(o), direction(d), mat(m), ttl(t) {};

    glm::vec3 origin, direction;
    int mat;
    int ttl;
};

struct Color{
    Color() = default;

    Color(float rr,float gg,float bb):r(rr),g(gg),b(bb){};

    // TODO: color correction
    inline Color& operator+=(Color const& c){
        r+=c.r; g+=c.g; b+=c.b;
        return *this;
    };

    inline Color& operator+=(float f){
        r+=f; g+=f; b+=f;
        return *this;
    };

    inline Color& operator*=(Color const& c){
        r*=c.r; g*=c.g; b*=c.b;
        return *this;
    }

    inline Color& operator*=(float f){ return *this*=Color(f,f,f); }

    inline bool isBlack() const {
        return !(r > 0.f && g > 0.f && b > 0.f);
    }

    float r,g,b;
};

inline Color operator*(Color a, float        b){ return a*=b; }
inline Color operator*(float a, Color        b){ return b*a; }
inline Color operator*(Color a, Color const& b){ return a*=b; }
inline Color operator+(Color a, Color const& b){ return a+=b; }


