#pragma once

#include "basics.h"
#include "scene.h"

#include "json.hpp"
#include "loadObject.h"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtx/io.hpp"

#include <fstream>
#include <string>
#include <vector>

using json = nlohmann::json;

inline glm::vec3 readXYZ(json const& o) {
    glm::vec3 result;
    result[0] = o["x"];
    result[1] = o["y"];
    result[2] = o["z"];
    return result;
}

inline glm::mat4 handleTransform(json const& o) {
    glm::mat4 result; // is initialised to identity

    if(o.find("translate") != o.end()){
        auto const& translate = readXYZ(o["translate"]);
        std::cout << "GOT TRANS " << translate << std::endl;
        result = glm::translate(result, translate);
    }

    if(o.find("rotate") != o.end()){
        auto const& rotate = readXYZ(o["rotate"]);
        std::cout << "GOT ROTATE" << rotate << std::endl;
    }

    if(o.find("scale") != o.end()){
        auto const& scale = readXYZ(o["scale"]);
        std::cout << "GOT SCALE " << scale << std::endl;
    }

    return result;
}

inline void handleObject(Scene& s, json const& o) {
    std::cout << o << std::endl;
    std::string kind = o["kind"];

    glm::mat4x4 transform; // initialised to identity
    // is there a transform for this obj?
    if (o.find("transform") != o.end()) {
        transform = handleTransform(o["transform"]);
        std::cout << "got transform " << transform << std::endl;
    }

    if(kind == "mesh"){

    }
    else if(kind == "sphere"){
        float radius = o["radius"];
        std::cerr << "sphere, radius = " << radius << std::endl;

        glm::vec4 starting(0, 0, 0, 1);
        glm::vec4 transformed = transform * starting;
        std::cout << "transformed " << transformed <<std::endl;
        glm::vec3 centre(transformed[0], transformed[1], transformed[2]);

    s.primitives.spheres.emplace_back(Sphere(centre, 3, radius));

    }
    else if(kind == "plane"){

    }
    else {
        std::cerr << "ERROR: object kind " << kind << " unknown\n";
    }
    
}

inline void handleLight(json const& l) {
    std::cout << l << std::endl;

}

inline bool loadScene(std::string const& filename, Scene& s) {

    std::ifstream inFile(filename);
    json o;
    inFile >> o;

#if 0
    // load meshes
    std::map<std::string, int> t;
    auto const& load_meshes = (o.find("load_meshes");
    if(o != o.end())) {
        const auto& meshes = *o;
        for(auto const& m: meshes.kv_map()) {
            std::string ref = m.first;
            std::string filename = m.second->get<jsonxx::String>(); 
            std::cout << ref << " " <<  filename << std::endl;
        }
    }
    else
        std::cout << "no meshes specified in scene\n";
#endif 

    auto const& world = o["world"];
    auto const& objects = world["objects"];

    for (auto const& object : objects) {
        handleObject(s, object);
    }

    auto const& lights = world["lights"];
    for (auto const& light: lights) {
        handleLight(light);
    }

    return true;
}

inline bool setupScene(Scene& s)
{
    const int red_glass = 1, tiles = 2, reflective_blue=3;

    try{
        loadScene("scene/test1.scene", s);
    } catch (std::exception const& e) {
        std::cerr << "exception loading scene - " << e.what() << std::endl;
        return false;
    }

#ifdef TEA_TIME_FOR_MRS_NESBIT 
    std::string filename = "obj/wt_teapot.obj";
    if(!loadObject(s, filename, reflective_blue)) // reflective_blue = pretty :)
        return false;
#endif

    // AIR
    s.primitives.materials.emplace_back(Color(0.f, 0.f,0.f),
                                        0.0f, 0.0f, 1.0f,
                                        1.0f);
    // RED GLASS
    s.primitives.materials.emplace_back(Color(0.2f, 0.8f, 0.8f),
                                        0.0f, 0.0f, 1.0f,
                                        1.5f);
    // WHITE/REFLECTIVE BLUE CHECKERED
    s.primitives.materials.emplace_back(Color(0.6f, 0.6f, 0.6f),
                                        1.0f, 0.0f, 0.0f,
                                        1.f, reflective_blue);
    // REFLECTIVE BLUE
    s.primitives.materials.emplace_back(Color(0.2f, 0.6f, 0.9f),
                                        0.8f, 0.2f, 0.0f,
                                        1.f);


    s.lights.emplace_back(glm::vec3(3,3,10), Color(10,10,10));
    s.lights.emplace_back(glm::vec3(-4,2,8), Color(10,10,10));
    s.lights.emplace_back(glm::vec3(2,4,15), Color(10,10,10));

    s.primitives.planes.emplace_back(glm::vec3(0,-1,0),  tiles, glm::vec3(0,1,0));

    return true;

    s.primitives.spheres.emplace_back(glm::vec3(0,1,10), red_glass, 1.9f);
    s.primitives.triangles.emplace_back(glm::vec3(1,0,24), 
                                        glm::vec3(-1,0,24), 
                                        glm::vec3(0,2,24), reflective_blue);


    return true;
}

