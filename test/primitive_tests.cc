#include "basics.h"
#include "primitive.h"
#include "glm/gtx/io.hpp"
#include <boost/test/unit_test.hpp>
#include <iostream>

BOOST_AUTO_TEST_CASE(ray_plane){
    Plane p(glm::vec3(10,0,0), Material(Color(0,0,0)) ,glm::vec3(-1,0,0));
    auto dist = p.distance(Ray(glm::vec3(0,0,0),glm::vec3(1,0,0), STARTING_TTL));
    BOOST_CHECK_EQUAL(dist,10.f);
}

BOOST_AUTO_TEST_CASE(ray_outsphere){
    OutSphere s(glm::vec3(10,0,0), Material(Color(0,0,0)), 2.f);
    auto dist = s.distance(Ray(glm::vec3(0,0,0),glm::vec3(1,0,0), STARTING_TTL));
    BOOST_CHECK_EQUAL(dist,8.f);
}
