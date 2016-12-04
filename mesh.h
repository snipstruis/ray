#pragma once

#include <vector>
#include "glm/vec3.hpp"

struct MeshTriangle {
    MeshTriangle(glm::vec3 const& v1, glm::vec3 const& v2, glm::vec3 const& v3)
        : verticies{v1, v2, v3} {} 

    glm::vec3 verticies[3];
};

struct Mesh {
    std::vector<MeshTriangle> triangles;
    int material;
};

