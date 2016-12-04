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

    inline Color& operator*=(Color const& c){
        r*=c.r; g*=c.g; b*=c.b;
        return *this;
    }

    inline Color& operator*=(float f){ return *this*=Color(f,f,f); }

    float r,g,b;
};

inline Color operator*(Color a, float        b){ return a*=b; }
inline Color operator*(float a, Color        b){ return b*a; }
inline Color operator*(Color a, Color const& b){ return a*=b; }
inline Color operator+(Color a, Color const& b){ return a+=b; }

struct Material{
    Material() = default;

    Material(Color c,
             float d, float r, float t, 
             float ri=1.f, int check=-1)
        : color(c),
        diffuseness(d), 
        reflectiveness(r), 
        transparency(t),
        refraction_index(ri),
        checkered(check){};

    Color color;
    float diffuseness;
    float reflectiveness;
    float transparency;
    float refraction_index;
    int checkered;
};

