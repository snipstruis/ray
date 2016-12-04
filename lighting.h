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
    SpotLight(glm::vec3 const& p, glm::vec3 const& dir, Color c, FalloffKind f, float angle): 
        pos(p), pointDir(dir), color(c), falloff(f), coneAngle(angle) {};

    glm::vec3 pos;
    glm::vec3 pointDir; // must be normalised
    Color color;
    FalloffKind falloff;
    float coneAngle;
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


