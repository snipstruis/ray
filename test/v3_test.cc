#include "v3.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(v3add){ 
    BOOST_CHECK_EQUAL(v3(1,2,3)+v3(2,3,4),v3(3,5,7));
    v3 a = v3(1,2,3);
    a += v3(2,3,4);
    BOOST_CHECK_EQUAL(a,v3(3,5,7));
}

BOOST_AUTO_TEST_CASE(v3sub){ 
    BOOST_CHECK_EQUAL(v3(2,3,4)-v3(1,2,3),v3(1,1,1));
    v3 a = v3(2,3,4);
    a -= v3(1,2,3);
    BOOST_CHECK_EQUAL(a,v3(1,1,1));
}

BOOST_AUTO_TEST_CASE(v3mul){ 
    BOOST_CHECK_EQUAL(2.f*v3(3),v3(6)); 
    BOOST_CHECK_EQUAL(v3(2)*3.f,v3(6));
    v3 a = v3(2,3,4);
    a *= 3;
    BOOST_CHECK_EQUAL(a,v3(6,9,12));
}

BOOST_AUTO_TEST_CASE(v3div){
    BOOST_CHECK_EQUAL(v3(3,6,12)/3,v3(1,2,4));
    v3 a = v3(3,6,12);
    a /= 3;
    BOOST_CHECK_EQUAL(a,v3(1,2,4));
}

BOOST_AUTO_TEST_CASE(v3dot){
    BOOST_CHECK_EQUAL(dot(v3(1,2,3),v3(4,5,6)), 32.f);
}

BOOST_AUTO_TEST_CASE(v3cross){
    BOOST_CHECK_EQUAL(cross(v3(1,2,3),v3(4,5,6)), v3(-3,6,-3));
}

BOOST_AUTO_TEST_CASE(v3lengthsq){
    BOOST_CHECK_EQUAL(lengthsq(v3(2,3,6)),7*7);
}

BOOST_AUTO_TEST_CASE(v3length){
    BOOST_CHECK_EQUAL(length(v3(2,3,6)),7);
}

BOOST_AUTO_TEST_CASE(v3distance){
    BOOST_CHECK_EQUAL(distance(v3(10,20,30),v3(12,23,36)),7);
}

BOOST_AUTO_TEST_CASE(v3normalize){
    BOOST_CHECK_EQUAL(normalize(v3(2,3,6)),v3(2/7.f,3/7.f,6/7.f));
}
