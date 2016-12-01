#include "camera.h"
#include "utils.h"
#include "debug_print.h"

#include "test/test_utils.h"

#include "glm/gtx/io.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtc/epsilon.hpp"

#include <boost/test/unit_test.hpp>

#include <iostream>

BOOST_AUTO_TEST_CASE(camera_setup_zero)
{
    // all defaults
    Camera c;
    
    BOOST_CHECK(VEC3_EQ(c.eye, glm::vec3(0, 0, 0)));
    BOOST_CHECK(VEC3_EQ(c.top_left, glm::vec3(-1, 1, 1)));
    BOOST_CHECK(VEC3_EQ(c.u, glm::vec3(2, 0, 0)));
    BOOST_CHECK(VEC3_EQ(c.v, glm::vec3(0, -2, 0)));
}

BOOST_AUTO_TEST_CASE(camera_setup_45fov)
{
    Camera c;
    c.fov = PI/3;
    c.buildCamera();
    
    // half side len is now 1/sqrt(3), and the vectors should be 2/sqrt(3) long
    float len = 1.0f / glm::root_three<float>();

    BOOST_CHECK(VEC3_EQ(c.eye, glm::vec3(0, 0, 0)));
    BOOST_CHECK(VEC3_EQ(c.top_left, glm::vec3(-len, len, 1)));
    BOOST_CHECK(VEC3_EQ(c.u, glm::vec3(2 * len , 0, 0)));
    BOOST_CHECK(VEC3_EQ(c.v, glm::vec3(0, -2 * len, 0)));
}

BOOST_AUTO_TEST_CASE(camera_rotation_yaw)
{
    Camera c;
    c.yaw = PI/2;
    c.buildCamera();

    BOOST_CHECK(VEC3_EQ(c.eye, glm::vec3(0, 0, 0)));
    BOOST_CHECK(VEC3_EQ(c.top_left, glm::vec3(1, 1, 1)));
    BOOST_CHECK(VEC3_EQ(c.u, glm::vec3(0, 0, -2)));
    BOOST_CHECK(VEC3_EQ(c.v, glm::vec3(0, -2, 0)));
}

BOOST_AUTO_TEST_CASE(camera_rotation_pitch)
{
    Camera c;
    c.pitch = -PI/2;
    c.buildCamera();

    BOOST_CHECK(VEC3_EQ(c.eye, glm::vec3(0, 0, 0)));
    BOOST_CHECK(VEC3_EQ(c.top_left, glm::vec3(-1, 1, -1)));
    BOOST_CHECK(VEC3_EQ(c.u, glm::vec3(2, 0, 0)));
    BOOST_CHECK(VEC3_EQ(c.v, glm::vec3(0, 0, 2)));
}

#if 0
BOOST_AUTO_TEST_CASE(tourist_info)
{
    // just dump out some basic info and test ostreams
    glm::vec3 foo;
    glm::mat4 bar;

    std::cout << "size of vec3 " << sizeof(foo) << std::endl;
    std::cout << "size of mat4 " << sizeof(bar) << std::endl;

    std::cout << foo << "\n";
    std::cout << bar << "\n\n";

    DumpToR(std::cout, foo);
    std::cout << std::endl;

    DumpToR(std::cout, bar);
    std::cout << std::endl;

    Ray r;
    std::cout << r << std::endl;
}
#endif
