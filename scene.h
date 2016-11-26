#pragma once

#include "glm/vec3.hpp"

#include "basics.h"
#include "primitive.h"

#include <vector>
#include <memory>

struct PointLight{
    PointLight(glm::vec3 p, Color c):pos(p),color(c){};
    glm::vec3 pos;
    Color color;
};

#include "camera.h"

struct Scene{
    Camera camera;
    Primitives primitives;
    std::vector<PointLight> lights;
};


