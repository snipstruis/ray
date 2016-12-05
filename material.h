#pragma once

#include "basics.h"
#include <vector>

const Color BLACK(0.f, 0.f, 0.f);

struct Material{
    Material() = default;

    Material(Color _diffuseColor,
             float r, 
             float t, 
             float ri=1.f, 
             int check=-1, 
             Color highlight=BLACK, 
             float shi=20.0f) :
        diffuseColor(_diffuseColor),
        reflectiveness(r), 
        transparency(t),
        refraction_index(ri),
        checkered(check),
        specular_highlight(highlight),
        shininess(shi){};

    Color diffuseColor;
    float reflectiveness;
    float transparency;
    float refraction_index;
    int checkered;
    Color specular_highlight; // brightness of specular highlight
    float shininess;          // the power to which te spec. highlight is raised
};

const int MATERIAL_AIR = 0;
const int MATERIAL_CHECKER = 1;
const int MATERIAL_REFLECTIVE_BLUE= 2;

inline void buildFixedMaterials(std::vector<Material>& v){
    // AIR
    v.emplace_back(Color(0.f, 0.f, 0.f), // diffuse col
                   0.0f,  // reflective
                   1.0f,  // transparency
                   1.0f); // refractive indexx

    // WHITE/REFLECTIVE BLUE CHECKERED
    v.emplace_back(Color(0.6f, 0.6f, 0.6f), // diffuse col
                   0.0f,  // reflective
                   0.0f,  // transparency
                   1.f,   // refractive indexx
                   MATERIAL_REFLECTIVE_BLUE);

    // REFLECTIVE BLUE
    v.emplace_back(Color(0.2f, 0.6f, 0.9f), // diffuse col
                   0.0f, // reflective
                   0.0f, // transparency
                   1.f,  // refractive index
                   -1,   // no checkerboard
                   Color(0.8f, 0.8f, 0.8f),  // specular highlight
                   32.f);  // shinyness
}

