#pragma once

#include "basics.h"
#include "scene.h"

#include "jsonxx.h"
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

inline bool setupScene(Scene& s)
{
    const int red_glass = 1, tiles = 2, reflective_blue=3;

#ifdef TEA_TIME_FOR_MRS_NESBIT 
    std::string filename = "obj/wt_teapot.obj";
    if(!loadObject(s, filename, reflective_blue)) // reflective_blue = pretty :)
        return false;
#endif

    // AIR
    s.primitives.materials.emplace_back(Color(0.f,0.f,0.f),
                                        0.0f, 0.0f, 1.0f,
                                        1.0f);
    // RED GLASS
    s.primitives.materials.emplace_back(Color(0.2,0.8,0.8),
                                        0.0f, 0.0f, 1.0f,
                                        1.5f);
    // WHITE/REFLECTIVE BLUE CHECKERED
    s.primitives.materials.emplace_back(Color(0.6,0.6,0.6),
                                        1.0f, 0.0f, 0.0f,
                                        1.f, reflective_blue);
    // REFLECTIVE BLUE
    s.primitives.materials.emplace_back(Color(0.2,0.6,0.9),
                                        0.8f, 0.2f, 0.0f,
                                        1.f);


    s.primitives.spheres.emplace_back(glm::vec3(0,1,10), red_glass, 1.9f);
    s.primitives.planes.emplace_back(glm::vec3(0,-1,0),  tiles, glm::vec3(0,1,0));
    s.primitives.triangles.emplace_back(glm::vec3(1,0,24), 
                                        glm::vec3(-1,0,24), 
                                        glm::vec3(0,2,24), reflective_blue);

    s.lights.emplace_back(glm::vec3(3,3,10), Color(10,10,10));
    s.lights.emplace_back(glm::vec3(-4,2,8), Color(10,10,10));
    s.lights.emplace_back(glm::vec3(2,4,15), Color(10,10,10));

    return true;
}

