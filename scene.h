#pragma once

#include "glm/vec3.hpp"

#include "basics.h"
#include "primitive.h"

#include <vector>
#include <memory>

struct PointLight{
    float r,g,b;
    glm::vec3 pos;
};

#include "camera.h"

struct Object {
};

struct Scene{
    Camera camera;
    std::vector<std::unique_ptr<Primitive>> primitives;
    std::vector<PointLight> lights;
};


