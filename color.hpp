#pragma once

#include "utils.hpp"

#include <cmath>
#include <ostream>

typedef glm::vec3 Color;
inline bool isBlack(Color c){return c==glm::vec3(0,0,0);};
inline Color clamp(Color c){return Color(clamp(c.r,0.f,1.f),
                                  clamp(c.g,0.f,1.f),
                                  clamp(c.b,0.f,1.f));}
inline Color scaleColor(glm::vec3 v, float base, float div){
    return Color((base+v.x)/div,
                 (base+v.y)/div,
                 (base+v.z)/div);
}
const Color BLACK(0.0f, 0.0f, 0.0f);
const Color WHITE(1.0f, 1.0f, 1.0f);
const Color PURPLE(1.0f, 0.0f, 1.0f);

