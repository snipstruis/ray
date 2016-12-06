#pragma once

#include "primitive.h"

#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtx/io.hpp"

#include <ostream>

// debug printing stuff
// keep it all in its own file so it can be un-included in one go
// note that glm already has operator<< for its types in glm/gtx/io.hpp

// code to dump glm objects, suitable for loading in R
inline std::ostream& DumpToR(std::ostream& os, const glm::mat4& m) {
    os << "matrix(c(";
    os <<  m[0][0] << ", " << m[1][0] << ", " << m[2][0] << ", " <<  m[3][0] << ", ";
    os <<  m[0][1] << ", " << m[1][1] << ", " << m[2][1] << ", " <<  m[3][1] << ", ";
    os <<  m[0][2] << ", " << m[1][2] << ", " << m[2][2] << ", " <<  m[3][2] << ", ";
    os <<  m[0][3] << ", " << m[1][3] << ", " << m[2][3] << ", " <<  m[3][3] ;
    os << "), nrow = 4, ncol = 4, byrow = TRUE)";
    return os;
}

inline std::ostream& DumpToR(std::ostream& os, const glm::vec3& m) {
    os << "c(";
    os <<  m[0] << ", " << m[1] << ", " << m[2];
    os << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Ray& r) {
    os << r.origin << " -> " << r.direction << " (ttl=" << r.ttl << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Intersection& i) {
    os << "dist " << i.distance;
    os << " impact " << i.impact;
    os << " normal " << i.normal;
    os << " internal " << i.internal;

    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Triangle& t) {
    os << "v=(0: " << t.v[0];
    os << " 1: " << t.v[1];
    os << " 2: " << t.v[2];
    os << ")\n n=(0:" << t.n[0];
    os << " 1: " << t.n[1];
    os << " 2: " << t.n[2];
    os << ")\n mat=" << t.mat << std::endl;

    return os;
}

