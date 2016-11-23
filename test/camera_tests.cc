#include "camera.h"
#include "debug_print.h"

#include "glm/gtx/io.hpp"

#include <boost/test/unit_test.hpp>

#include <iostream>

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

BOOST_AUTO_TEST_CASE(camera_setup_zero)
{
    Camera c;
}
