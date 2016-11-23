#pragma once

#include "basics.h"

#include "glm/vec3.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/glm.hpp"

#include <cassert>

const int STARTING_TTL = 10; // probably should be configurable or dynamically calculated

struct Camera{
    // screen res in pixels
    int width, height;
    // eye pos in world space
    glm::vec3 eye;
    // point at top left of screen in world space
    glm::vec3 top_left;
    // u (horiz) and v (vertical) vectors from top_left point
    glm::vec3 u, v;

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
        assert(width > 0);
        
        sanityCheck();

        // convert pixel space to percent across the screen
        float px = (((float)x)/width);
        float py = (((float)y)/width);

        // using pre-existing u & v vectors, get a point on the screen in world space
        glm::vec3 screen_point = top_left + (px * u) + (py * v);

        // and a vector from the eye to the screen point
        glm::vec3 look_vec = screen_point - eye;
        
        // build result - note direction is normalised
        return Ray(eye, glm::normalize(look_vec), STARTING_TTL);
    };

    void sanityCheck() const
    {
        // check that the camera/screen has been set up - ie buildCamera has been called
        // ... this will fail if buildCamera's not been called.
        assert(glm::length(u) > 0);
        assert(glm::length(v) > 0);

        // u & v should be perpendicular
        // FIXME: using == (naughty)
        assert(glm::dot(u, v) == 0);
    }

    // check an angle is clamped 0 <= angle < 2pi (ie within one rotation)
    bool isAngleInOneRev(float angle)
    {
        const float pi = glm::pi<float>();
        return angle >= 0 && angle < 2*pi;
    }

    bool isAngleInHalfRev(float angle)
    {
        const float pi = glm::pi<float>();
        return angle >= 0 && angle < pi;
    }

    // FIXME: probably break this into a functon to move the eye and change the other params 
    // separately, as they are triggered from separate inputs
    // y/p/r must be <2pi, fov < pi
    void buildCamera(const glm::vec3& _eye, float yaw, float pitch, float roll, float fov)
    {
        assert(isAngleInOneRev(yaw));
        assert(isAngleInOneRev(pitch));
        assert(isAngleInOneRev(roll));
        assert(isAngleInHalfRev(fov));

        // start with 3x points around the screen
        auto tl = glm::vec3(1, 1, -1);
        auto bl = glm::vec3(1, -1, -1);
        auto tr = glm::vec3(1, 1, 1);


#if 0

        // get vector for look angle - ie handle yaw & pitch
        auto look_dir = glm::vec3(
            glm::cos(yaw) * glm::cos(pitch), // x
            glm::sin(pitch),                 // y
            glm::sin(yaw) * glm::cos(pitch));// z

        // FIXME: don't use ==
        assert(glm::length(look_dir) == 0.0);
#endif
        top_left = tl;
        u = tr - tl;
        v = bl - tl;

        // keep a copy of eye
        eye = _eye;

        // top_left is eye 
        sanityCheck();
    }
};
