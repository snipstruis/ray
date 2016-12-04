#pragma once

#include "glm/vec3.hpp"

#include <map>
#include <string>
#include <vector>

struct MeshTriangle {
    MeshTriangle(glm::vec3 const& _v1, glm::vec3 const& _v2, glm::vec3 const& _v3)
        : v1(_v1), v2(_v2), v3(_v3) {} 

    glm::vec3 v1, v2, v3;
};

struct Mesh {
    std::vector<MeshTriangle> triangles;
    int material;
};

typedef std::map<std::string, Mesh> MeshMap;

