#pragma once

#include "utils.h"

#include <cmath>

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

    bool isFinite() const {
        return std::isfinite(r) && std::isfinite(g) && std::isfinite(b);
    }

    static bool valLegal(float val) {
        return std::isfinite(val) && val >= 0.0f && val <= 1.0f;
    }

    // Check col is legal for final output (ie [0.0-1.0])
    bool isLegal() const {
        return valLegal(r) && valLegal(g) && valLegal(b);
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

