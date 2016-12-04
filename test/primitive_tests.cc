#include "basics.h"
#include "primitive.h"
#include "debug_print.h"

#include "glm/gtx/io.hpp"

#include <boost/test/unit_test.hpp>
#include <iostream>

#if 0
BOOST_AUTO_TEST_CASE(ray_plane){
    Plane p(glm::vec3(10,0,0), Material(Color(0,0,0),0) ,glm::vec3(-1,0,0));
    auto dist = p.distance(Ray(glm::vec3(0,0,0),glm::vec3(1,0,0), STARTING_TTL));
    BOOST_CHECK_EQUAL(dist,10.f);
}
#endif

BOOST_AUTO_TEST_CASE(sphereIntersect){
    Sphere sphere(glm::vec3(0,0,0), 0, 2.f);

    {
        Ray r(glm::vec3(0, 0, -5), glm::vec3(0, 0, 1), 0, STARTING_TTL);
        auto i = intersect(sphere, r);
        std::cout << i<< std::endl;
    }

    {
        Ray r(glm::vec3(0, 0, 5), glm::vec3(0, 0, -1), 0, STARTING_TTL);
        auto intersection = intersect(sphere, r);
        std::cout << intersection << std::endl;
    }

    {
        Ray r(glm::vec3(-5, 0, 0), glm::vec3(1, 0, 0), 0, STARTING_TTL);
        auto intersection = intersect(sphere, r);
        std::cout << intersection << std::endl;
    }

    {
        Ray r(glm::vec3(5, 0, 0), glm::vec3(-1, 0, 0), 0, STARTING_TTL);
        auto intersection = intersect(sphere, r);
        std::cout << intersection << std::endl;
    }

    {
        Ray r(glm::vec3(0, -5, 0), glm::vec3(0, 1, 0), 0, STARTING_TTL);
        auto intersection = intersect(sphere, r);
        std::cout << intersection << std::endl;
    }

    {
        Ray r(glm::vec3(0, 5, 0), glm::vec3(0, -1, 0), 0, STARTING_TTL);
        auto intersection = intersect(sphere, r);
        std::cout << intersection << std::endl;
    }
}
