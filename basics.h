#pragma once

#include "glm/vec3.hpp"
#include "glm/glm.hpp"

// FIXME: looking for a better name for this file..
// Basic types, used in most places 

const int STARTING_TTL = 10; // probably should be configurable or dynamically calculated

struct Ray{
    Ray(glm::vec3 o, glm::vec3 d, int t):origin(o),direction(d),ttl(t){};
    glm::vec3 origin, direction;
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
    inline Color& operator*=(Color const& c){
        r*=c.r; g*=c.g; b*=c.b;
        return *this;
    }
    inline Color& operator*=(float f){ return *this+=Color(f,f,f); }
    float r,g,b;
};

inline Color operator*(Color a, float        b){ return a*=b; }
inline Color operator*(Color a, Color const& b){ return a*=b; }
inline Color operator+(Color a, Color const& b){ return a+=b; }

enum : unsigned {
    MAT_lit          = 1<<0,
    MAT_diffuse      = 1<<1,
    MAT_shadow       = 1<<2,
    MAT_specular     = 1<<3,
    MAT_transparancy = 1<<4,
};

struct Material{
    Material() = default;
    Material(Color c,unsigned p):color(c),properties(p){};
    Color color;
    unsigned properties;
};

