#pragma once

#include "glm/vec3.hpp"

#include "basics.h"
#include "primitive.h"

#include <vector>

struct PointLight{
    float r,g,b;
    glm::vec3 pos;
};

#include "camera.h"

struct Object {
};

struct Scene{
    Camera camera;
    std::vector<Object> objects;
    std::vector<PointLight> lights;
};

