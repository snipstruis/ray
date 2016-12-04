#pragma once

#include "glm/vec3.hpp"

#include "basics.h"
#include "camera.h"
#include "lighting.h"
#include "primitive.h"

#include <vector>

struct PointLight{
    PointLight(glm::vec3 p, Color c):pos(p),color(c){};
    glm::vec3 pos;
    Color color;
};

struct Lights {
    std::vector<PointLight> pointLights;
};

struct Scene{
    Camera camera;
    Primitives primitives;
    Lights lights;
};


