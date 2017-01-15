#pragma once

#include "basics.h"
#include "color.h"
#include "lighting.h"

#include "glm/vec3.hpp"
#include <vector>

enum FalloffKind {
    FK_LINEAR,
    FK_LOG
};

struct SpotLight{
    SpotLight(
        const glm::vec3& p, 
        const glm::vec3& dir, 
        const Color& c, 
        FalloffKind f, 
        float _innerAngle, 
        float _outerAngle) 
        : 
        pos(p), 
        pointDir(dir), 
        color(c), 
        falloff(f), 
        cosInnerAngle(cosf(_innerAngle)), 
        cosOuterAngle(cosf(_outerAngle)) 
    {
    }

    glm::vec3 pos;
    glm::vec3 pointDir; // must be normalised
    Color color;
    FalloffKind falloff;
    // cosine of the inner and outer angle of the light 
    float cosInnerAngle;
    float cosOuterAngle;
};

struct PointLight{
    PointLight(glm::vec3 const& p, Color c) : pos(p), color(c) {};

    glm::vec3 pos;
    Color color;
};

struct Lights {
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;
};


