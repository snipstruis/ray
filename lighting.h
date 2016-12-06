#pragma once

#include "basics.h"
#include "lighting.h"

#include "glm/vec3.hpp"
#include <vector>

enum FalloffKind {
    FK_LINEAR,
    FK_LOG
};

struct SpotLight{
    SpotLight(glm::vec3 const& p, glm::vec3 const& dir, Color c, FalloffKind f, float _innerAngle, float _outerAngle): 
        pos(p), pointDir(dir), color(c), falloff(f), innerAngle(_innerAngle), outerAngle(_outerAngle) {};

    glm::vec3 pos;
    glm::vec3 pointDir; // must be normalised
    Color color;
    FalloffKind falloff;
    float innerAngle;
    float outerAngle;
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


