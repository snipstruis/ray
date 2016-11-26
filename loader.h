#pragma once

#include "basics.h"
#include "scene.h"
#include "glm/vec3.hpp"

inline void setupScene(Scene& s)
{
    unsigned prop = MAT_checkered | MAT_diffuse | MAT_shadow;
    s.primitives.materials.emplace_back(Material(Color(0.6,0.5,0.4),
                                       prop|MAT_specular|MAT_transparent,0.7f,1.52f));
    s.primitives.materials.emplace_back(Material(Color(0.6,0.6,0.6),
                                       prop));
    s.primitives.spheres.emplace_back(glm::vec3(0,0,10),0, 2.f);
    s.primitives.planes.emplace_back(Plane(glm::vec3(0,-1,0), 1, glm::vec3(0,1,0)));
    s.primitives.triangles.emplace_back(Triangle(glm::vec3(1,0,14), glm::vec3(-1,0,14), glm::vec3(0,2,14),1));
    s.lights.emplace_back(glm::vec3(3,3,10), Color(6,6,6));
    s.lights.emplace_back(glm::vec3(-4,2,8), Color(10,2,2));
    s.lights.emplace_back(glm::vec3(2,4,15), Color(2,10,2));
}

