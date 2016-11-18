#include "linalg_ref.h"

#define BOOST_TEST_MODULE MyTest
#include <boost/test/unit_test.hpp>

namespace la { namespace test {

BOOST_AUTO_TEST_CASE(VectorTest)
{
    ref::vec3 v(0,1,0);
    BOOST_CHECK_EQUAL(Length(v), 1);
}

BOOST_AUTO_TEST_CASE(DotProductTest)
{
    ref::vec3 v1(0,1,0);
    ref::vec3 v2(1,0,0);

    BOOST_CHECK_EQUAL(ref::DotProduct(v1, v2), 0);
    BOOST_CHECK_EQUAL(ref::DotProduct(v1, v1), 1);
}
}} // namespaces
