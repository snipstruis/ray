#pragma once

#include "primitive.h"

#include "glm/vec3.hpp"

#include <map>
#include <string>
#include <vector>

struct Triangle{
    Triangle(
            glm::vec3 const& _v1, glm::vec3 const& _v2, glm::vec3 const& _v3, 
            glm::vec3 const& _n1, glm::vec3 const& _n2, glm::vec3 const& _n3, 
            int _material) :
        v{_v1, _v2, _v3}, 
        n{_n1, _n2, _n3}, 
        mat(_material) { } 
    glm::vec3 v[3];
    // per-vertex normal
    glm::vec3 n[3];
    // material
    int mat; 
};

struct Mesh {
    std::vector<Triangle> triangles;
};

typedef std::map<std::string, Mesh> MeshMap;

