#pragma once

#include <ostream>
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"

// debug printing stuff
// keep it all in its own file so it can be un-included in one go
// note that glm already has operator<< for its types in glm/gtx/io.hpp

std::ostream& DumpToR(std::ostream& os, const glm::mat4& m) {
    os << "matrix(c(";
    os <<  m[0][0] << ", " << m[1][0] << ", " << m[2][0] << ", " <<  m[3][0] << ", ";
    os <<  m[0][1] << ", " << m[1][1] << ", " << m[2][1] << ", " <<  m[3][1] << ", ";
    os <<  m[0][2] << ", " << m[1][2] << ", " << m[2][2] << ", " <<  m[3][2] << ", ";
    os <<  m[0][3] << ", " << m[1][3] << ", " << m[2][3] << ", " <<  m[3][3] ;
    os << "), nrow = 4, ncol = 4, byrow = TRUE)";
    return os;
}

std::ostream& DumpToR(std::ostream& os, const glm::vec3& m) {
    os << "c(";
    os <<  m[0] << ", " << m[1] << ", " << m[2];
    os << ")";
    return os;
}

