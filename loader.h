#pragma once

#include "basics.h"
#include "scene.h"
#include "glm/vec3.hpp"

inline void setupScene(Scene& s)
{
    s.primitives.materials.emplace_back(Color(0.8,0.2,0.2),
                                        0.0f, 0.0f, 1.0f,
                                        1.5f,-1);
    s.primitives.materials.emplace_back(Color(0.6,0.5,0.4),
                                        0.3f, 0.7f, 0.0f,
                                        1.5f);
    s.primitives.materials.emplace_back(Color(0.6,0.6,0.6),
                                        1.0f, 0.0f, 0.0f,
                                        1.f, 3);
    s.primitives.materials.emplace_back(Color(0.2,0.6,0.9),
                                        0.8f, 0.2f, 0.0f);
    s.primitives.spheres.emplace_back(glm::vec3(0,0,10),0, 2.f);
    s.primitives.planes.emplace_back(glm::vec3(0,-1,0), 2, glm::vec3(0,1,0));
    s.primitives.triangles.emplace_back(glm::vec3(1,0,14), 
                                        glm::vec3(-1,0,14), 
                                        glm::vec3(0,2,14),2);
    s.lights.emplace_back(glm::vec3(3,3,10), Color(6,6,6));
    s.lights.emplace_back(glm::vec3(-4,2,8), Color(10,2,2));
    s.lights.emplace_back(glm::vec3(2,4,15), Color(2,10,2));
}

