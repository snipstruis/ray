#pragma once

#include "utils.h"

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

inline Color ColorClamp(Color c, float min, float max) {
    return Color(clamp(c.r, min, max), clamp(c.g, min, max), clamp(c.b, min, max));
}

