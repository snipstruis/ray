#pragma once

#include "camera.h"
#include "lighting.h"
#include "primitive.h"

struct Scene{
    Camera camera;
    Primitives primitives;
    Lights lights;
};


