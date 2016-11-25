#pragma once

#include "basics.h"
#include "utils.h"

#include "glm/vec3.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/glm.hpp"
#include "glm/gtx/epsilon.hpp"
#include "glm/gtx/rotate_vector.hpp"

#include "test/test_utils.h"

#include <cassert>
#include <iostream>

// inputs to a render, but don't really belong in the Camera itself
// should an input to eye belong here?
struct RenderParams{
    // note: all angles in radians
    RenderParams() :
        yaw(0), pitch(0), roll(0), fov(PI/2) {}

    float yaw, pitch, roll, fov;
};

struct Camera{
    // screen res in pixels
    int width, height;
    // eye pos in world space
    glm::vec3 eye;
    // point at top left of screen in world space
    glm::vec3 top_left;
    // u (horiz) and v (vertical) vectors from top_left point
    glm::vec3 u, v;

    Camera() : width(0), height(0){
        buildCamera(RenderParams());
    }

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
        // FIXME: really should be epsilon compare
        assert(glm::dot(u, v) ==  0);
    }

    // check an angle is clamped -2pi < angle < 2pi (ie within one rotation either way)
    bool isAngleInOneRev(float angle)
    {
        return angle > -2*PI  && angle < 2*PI;
    }

    bool isAngleInHalfRev(float angle)
    {
        return angle >= 0 && angle < PI;
    }

    // FIXME: probably break this into a functon to move the eye and change the other params 
    // separately, as they are triggered from separate inputs
    // y/p/r must be <2pi, fov < pi
    //void buildCamera(const glm::vec3& _eye, float yaw, float pitch, float roll, float fov)
    void buildCamera(RenderParams const& rp)
    {
        assert(isAngleInOneRev(rp.yaw));
        assert(isAngleInOneRev(rp.pitch));
        assert(isAngleInOneRev(rp.roll));
        assert(isAngleInHalfRev(rp.fov));

        // start with 3x points around the screen
        auto tl = glm::vec3(-1, 1, 1);
        auto bl = glm::vec3(-1, -1, 1);
        auto tr = glm::vec3(1, 1, 1);

        // adjust for fov, keeping dist along z axis const
        float fov_ratio = glm::tan(rp.fov/2); 
        auto fov_adj = glm::vec3(fov_ratio, fov_ratio, 1);
        
        tl = tl * fov_adj; 
        bl = bl * fov_adj; 
        tr = tr * fov_adj; 

        // pitch and yaw.. note if done in this order, doesn't require a composed translation
        tl = glm::rotateX(tl, rp.pitch);
        bl = glm::rotateX(bl, rp.pitch);
        tr = glm::rotateX(tr, rp.pitch);

        tl = glm::rotateY(tl, rp.yaw);
        bl = glm::rotateY(bl, rp.yaw);
        tr = glm::rotateY(tr, rp.yaw);

        // now all is said and done, calc relative vectors
        u = tr - tl;
        v = bl - tl;

        // transform top left to world
        top_left = tl + eye;

        sanityCheck();
    }
};
