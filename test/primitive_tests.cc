#include "primitive.h"
#include <boost/test/unit_test.hpp>
#include <iostream>

/* 
std::ostream& operator<<(std::ostream& os, const glm::vec3& v) {  
    os << '(' << v.x << ", " << v.y << ", " << v.z << ')';
    return os;  
}
*/

BOOST_AUTO_TEST_CASE(ray_plane){
    Plane p;
    p.pos = glm::vec3(10,0,0);
    p.norm = glm::vec3(-1,0,0);
    auto dist = p.distance(Ray(glm::vec3(0,0,0),glm::vec3(1,0,0)));
    BOOST_CHECK_EQUAL(dist,10.f);
}

BOOST_AUTO_TEST_CASE(ray_outsphere){
    OutSphere s;
    s.pos = glm::vec3(10,0,0);
    s.radius = 2.f;
    auto dist = s.distance(Ray(glm::vec3(0,0,0),glm::vec3(1,0,0)));
    BOOST_CHECK_EQUAL(dist,8.f);
}
