#pragma once

#include "utils.h"

#include "glm/gtc/epsilon.hpp"
#include "glm/gtx/io.hpp"

#include <boost/test/predicate_result.hpp>

boost::test_tools::predicate_result VEC3_EQ(glm::vec3 const& a, glm::vec3 const& b) {
    if (!glm::all(glm::epsilonEqual(a, b, EPSILON))){
        boost::test_tools::predicate_result r = false;
        r.message() << a << " != " << b;
        return r;
    }
    else
        return true;
}
