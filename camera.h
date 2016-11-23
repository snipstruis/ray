#pragma once

#include "basics.h"

#include "glm/vec3.hpp"
#include <cassert>

struct Camera{
    int width, height;
    glm::vec3 eye, screen_center, screen_corner;

    Camera() : width(0), height(0) {}

    // takes a screen co-ord, and returns a ray from the camera through that pixel
    // note that this is in screen space and not world space, the conversion is handled internally
    Ray makeRay(int x, int y)
    {
        assert(x >= 0);
        assert(y >= 0);
        assert(x < width);
        assert(y < height);
        // check the width and height have been set
        assert(height > 0);
        assert(width> 0);


        return Ray(); // silence warning for now
    };
};
