#pragma once

#include "glm/vec3.hpp"

#include <map>
#include <string>
#include <vector>

struct MeshTriangle {
    MeshTriangle(
            glm::vec3 const& _v1, glm::vec3 const& _v2, glm::vec3 const& _v3, 
            glm::vec3 const& _n1, glm::vec3 const& _n2, glm::vec3 const& _n3, 
            int _material) :
        v1(_v1), v2(_v2), v3(_v3), 
        n1(_n1), n2(_n2), n3(_n3), 
        material(_material) {} 
    
    // call if you don't have normals - n1,n2,n3 will be init'd to 0,0,0
    // this version might get removed in future.
    MeshTriangle(
            glm::vec3 const& _v1, glm::vec3 const& _v2, glm::vec3 const& _v3, 
            int _material) :
        v1(_v1), v2(_v2), v3(_v3), 
        material(_material) {} 

    // verticies
    glm::vec3 v1, v2, v3;
    // vertex normals
    glm::vec3 n1, n2, n3;
    int material;
};

struct Mesh {
    std::vector<MeshTriangle> triangles;
};

typedef std::map<std::string, Mesh> MeshMap;

