#pragma once

#include "basics.h"
#include "utils.h"

#include "glm/vec3.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/glm.hpp"
#include "glm/gtx/epsilon.hpp"
#include "glm/gtx/rotate_vector.hpp"

#include "test/test_utils.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>

struct Camera{
    // screen res in pixels
    int width, height;

    // These are the render-time params - ie calculated once per frame(max) to save work
    // eye pos in world space
    glm::vec3 eye;
    // point at top left of screen in world space
    glm::vec3 top_left;
    // u (horiz) and v (vertical) vectors from top_left point
    glm::vec3 u, v;
    
    // These are the input-params - ie changed by user input, and rendered into the params above by buildCamera()
    float yaw, pitch, roll, fov;

    Camera() : width(0), height(0), yaw(0), pitch(0), roll(0), fov(PI/2) {
        buildCamera();
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
        float px = ((float)x)/width;
        float py = ((float)y)/width;

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
        assert(feq(glm::dot(u, v), 0.0f));
    }
    // FIXME: probably break this into a functon to move the eye and change the other params 
    // separately, as they are triggered from separate inputs
    // y/p/r must be <2pi, fov < pi
    void buildCamera()
    {
        assert(isAngleInOneRev(yaw));
        assert(isAngleInOneRev(pitch));
        assert(isAngleInOneRev(roll));
        assert(isAngleInHalfRev(fov));

        // start with 3x points around the screen
        auto tl = glm::vec3(-1, 1, 1);
        auto bl = glm::vec3(-1, -1, 1);
        auto tr = glm::vec3(1, 1, 1);

        // adjust for fov, keeping dist along z axis const
        float fov_ratio = glm::tan(fov/2); 
        auto fov_adj = glm::vec3(fov_ratio, fov_ratio, 1);
        
        tl = tl * fov_adj; 
        bl = bl * fov_adj; 
        tr = tr * fov_adj; 

        // pitch and yaw.. note if done in this order, doesn't require a composed translation
        tl = glm::rotateX(tl, pitch);
        bl = glm::rotateX(bl, pitch);
        tr = glm::rotateX(tr, pitch);

        tl = glm::rotateY(tl, yaw);
        bl = glm::rotateY(bl, yaw);
        tr = glm::rotateY(tr, yaw);

        // now all is said and done, calc relative vectors
        u = tr - tl;
        v = bl - tl;

        // transform top left to world
        top_left = tl + eye;

        sanityCheck();
    }

    // safe delta functions - from user input
    void moveFov(float d) {
        fov = clamp(fov + d, EPSILON, PI-EPSILON);
    }

    // move forwards/backwards (neg = right)
    void moveForward(float d) {
        eye[2] += d;
    }

    // move left/right (neg = right)
    void moveRight(float d) {
        eye[0] += d;
    }

    void moveYawPitch(float deltaYaw, float deltaPitch){
        yaw = happyfMod(yaw + deltaYaw, PI*2);
        // pitch is limited to look directly up / down (ie no upside down)
        pitch = clamp(pitch + deltaPitch, -PI/2, PI/2);
        std::cout<< "MYP " << yaw << " " << pitch<< std::endl;
    }
};
