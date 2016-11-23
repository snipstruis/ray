#include "camera.h"
#include "debug_print.h"

#include "glm/gtx/io.hpp"

#include <boost/test/unit_test.hpp>

#include <iostream>

BOOST_AUTO_TEST_CASE(camera_test)
{
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
	
}
