#include "camera.h"
#include "utils.h"
#include "debug_print.h"

#include "test/test_utils.h"

#include "glm/gtx/io.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtc/epsilon.hpp"

#include <boost/test/unit_test.hpp>

#include <iostream>

void setDefaults(Camera& c) {
    c.startingOrigin = glm::vec3(0, 0, 0);
    c.startingPitch = 0;
    c.startingYaw = 0;
    c.startingFov = PI/2;

    c.resetView();
}

BOOST_AUTO_TEST_CASE(camera_setup_zero)
{
    // all defaults
    Camera c;
    setDefaults(c);
    
    BOOST_CHECK(VEC3_EQ(c.origin, glm::vec3(0, 0, 0)));
    BOOST_CHECK(VEC3_EQ(c.top_left, glm::vec3(-1, 1, 1)));
    BOOST_CHECK(VEC3_EQ(c.u, glm::vec3(2, 0, 0)));
    BOOST_CHECK(VEC3_EQ(c.v, glm::vec3(0, -2, 0)));
}

BOOST_AUTO_TEST_CASE(camera_setup_45fov)
{
    Camera c;
    setDefaults(c);

    c.fov = PI/3;
    c.buildCamera();
    
    // half side len is now 1/sqrt(3), and the vectors should be 2/sqrt(3) long
    float len = 1.0f / glm::root_three<float>();

    BOOST_CHECK(VEC3_EQ(c.origin, glm::vec3(0, 0, 0)));
    BOOST_CHECK(VEC3_EQ(c.top_left, glm::vec3(-len, len, 1)));
    BOOST_CHECK(VEC3_EQ(c.u, glm::vec3(2 * len , 0, 0)));
    BOOST_CHECK(VEC3_EQ(c.v, glm::vec3(0, -2 * len, 0)));
}

BOOST_AUTO_TEST_CASE(camera_rotation_yaw)
{
    Camera c;
    setDefaults(c);

    c.yaw = PI/2;
    c.buildCamera();

    BOOST_CHECK(VEC3_EQ(c.origin, glm::vec3(0, 0, 0)));
    BOOST_CHECK(VEC3_EQ(c.top_left, glm::vec3(1, 1, 1)));
    BOOST_CHECK(VEC3_EQ(c.u, glm::vec3(0, 0, -2)));
    BOOST_CHECK(VEC3_EQ(c.v, glm::vec3(0, -2, 0)));
}

BOOST_AUTO_TEST_CASE(camera_rotation_pitch)
{
    Camera c;
    setDefaults(c);

    c.pitch = -PI/2;
    c.buildCamera();

    BOOST_CHECK(VEC3_EQ(c.origin, glm::vec3(0, 0, 0)));
    BOOST_CHECK(VEC3_EQ(c.top_left, glm::vec3(-1, 1, -1)));
    BOOST_CHECK(VEC3_EQ(c.u, glm::vec3(2, 0, 0)));
    BOOST_CHECK(VEC3_EQ(c.v, glm::vec3(0, 0, 2)));
}

