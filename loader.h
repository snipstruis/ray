#pragma once

#include "basics.h"
#include "scene.h"

#include "tiny_obj_loader.h"
#include "glm/vec3.hpp"
#include <string>
#include <vector>

glm::vec3 makeVec3FromVerticies(tinyobj::attrib_t const& attrib, int index) {
    int i = index * 3;
    std::cout << "idx " << (i/3)+1<< std::endl;
    glm::vec3 result(attrib.vertices[i], attrib.vertices[i + 1], attrib.vertices[i + 2]);
    std::cout <<result << std::endl;
    return result;
}

bool loadObject(Scene& s){
    std::string filename = "/Users/nick/src/u/ray/obj/wt_teapot.obj";

    std::string err;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str());
    if(!ret)
        return false;
        
    std::cout << "ret " << ret << std::endl;
    std::cout << "err " << err<< std::endl;
    std::cout << "nshapes " << shapes.size()<< std::endl;

    //for(int i = 0; i < attrib.vertices.size(); i++) {
   //     if (i%3==0)
     //       std::cout<<"\n" << i << ": ";
      //  std::cout << attrib.vertices[i] << " ";
   // }
    std::cout<<std::endl;
    for(auto const& shape : shapes) {
 //       std::cout << "shape " << shape.name << std::endl;
 //       std::cout << "mesh size" << shape.mesh.indices.size() << std::endl;

        // tinyobj should tesselate for us.
        assert(shape.mesh.indices.size() % 3 == 0);

        int ntriangles = shape.mesh.indices.size() / 3;
 //       std::cout << "ntriangles " << ntriangles << std::endl;

        for (int i = 0; i < ntriangles; i++) {
            int base = i * 3; 
            Triangle t(
                makeVec3FromVerticies(attrib, shape.mesh.indices[base].vertex_index),
                makeVec3FromVerticies(attrib, shape.mesh.indices[base + 1].vertex_index),
                makeVec3FromVerticies(attrib, shape.mesh.indices[base + 2].vertex_index),
                0);
            s.primitives.triangles.emplace_back(t);
            std::cout << std::endl;
        }
    }

    std::cout << "load done " << std::endl;
//    exit(1);

    return true;
}

inline bool setupScene(Scene& s)
{
    if(!loadObject(s))
        return false;

    s.primitives.materials.emplace_back(Material(Color(0.6,0.5,0.4),
                                                 0.3f, 0.7f, 0.0f,
                                                 1.52f, 1));
    s.primitives.materials.emplace_back(Material(Color(0.6,0.5,0.4),
                                                 0.7f, 0.3f, 0.0f,
                                                 1.52f));
    s.primitives.materials.emplace_back(Material(Color(0.6,0.6,0.6),
                                                 1.0f, 0.0f, 0.0f,
                                                 1.f, 3));
    s.primitives.materials.emplace_back(Material(Color(0.2,0.6,0.9),
                                                 0.8f, 0.2f, 0.0f));

    s.primitives.spheres.emplace_back(glm::vec3(2,2,-2),0, 2.f);

    s.primitives.planes.emplace_back(Plane(glm::vec3(0,-1,0), 2, glm::vec3(0,1,0)));
//    s.primitives.triangles.emplace_back(Triangle(glm::vec3(1,0,14), glm::vec3(-1,0,14), glm::vec3(0,2,14),2));
    s.lights.emplace_back(glm::vec3(3,3,10), Color(6,6,6));
    s.lights.emplace_back(glm::vec3(-4,2,8), Color(10,2,2));
    s.lights.emplace_back(glm::vec3(2,4,15), Color(2,10,2));

    return true;
}

