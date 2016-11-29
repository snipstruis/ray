#pragma once

#include "basics.h"
#include "scene.h"

#include "tiny_obj_loader.h"
#include "glm/vec3.hpp"

#include <string>
#include <vector>

// all the verticies from tinyobj are in a flat array, in groups of 3.
// this function grabs 3 consecutive points and builds a vec3
// index is the tuple number - ie index 2 starts at array index 6
glm::vec3 makeVec3FromVerticies(tinyobj::attrib_t const& attrib, int index) {
    int i = index * 3;
    glm::vec3 result(attrib.vertices[i], attrib.vertices[i + 1], attrib.vertices[i + 2]);
    return result;
}

bool loadObject(Scene& s, std::string const& filename, int material){
    std::string err;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::cout << "loading " << filename << std::endl;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str());
    if(!ret) {
        std::cout << "ERROR loading object - " << err << std::endl;
        return false;
    }
        
    for(auto const& shape : shapes) {
        // tinyobj should tesselate for us.
        assert(shape.mesh.indices.size() % 3 == 0);

        int ntriangles = shape.mesh.indices.size() / 3;

        for (int i = 0; i < ntriangles; i++) {
            int base = i * 3; 
            Triangle t(
                makeVec3FromVerticies(attrib, shape.mesh.indices[base].vertex_index),
                makeVec3FromVerticies(attrib, shape.mesh.indices[base + 1].vertex_index),
                makeVec3FromVerticies(attrib, shape.mesh.indices[base + 2].vertex_index),
                material);
            s.primitives.triangles.emplace_back(t);
        }
    }

    std::cout << "load done" << std::endl;

    return true;
}

