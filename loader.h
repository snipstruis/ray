#pragma once

#include "basics.h"
#include "scene.h"

#include "jsonxx.h"
#include "loadObject.h"
#include "glm/vec3.hpp"

#include <fstream>
#include <string>
#include <vector>

inline void handleTransform(jsonxx::Object const& o) {

}

inline bool handleObject(jsonxx::Object const& o) {
    std::cout << o << std::endl;
    if (!o.has<jsonxx::String>("kind")) {
        std::cerr << "ERROR: object missing kind string\n";
        return false;
    }

    const std::string& kind = o.get<jsonxx::String>("kind");

    if(kind == "mesh"){
    }
    else if(kind == "sphere"){
        if(!o.has<jsonxx::Number>("radius")){
            std::cerr << "ERROR: sphere missing radius\n";
            return false;
        }
        float radius = o.get<jsonxx::Number>("radius");
        std::cerr << "sphere, radius = " << radius << std::endl;

    }
    else if(kind == "plane"){
    }
    else {
        std::cerr << "ERROR: object kind " << kind << " unknown\n";
        return false;
    }
    
    // is there a transform for this obj?
    if (!o.has<jsonxx::Object>("transform")) {
        handleTransform(o.get<jsonxx::Object>("transform"));
    }

    return true;
}

inline bool loadScene(std::string const& filename ,Scene& s) {
    std::ifstream inFile(filename);

    jsonxx::Object o;
    if(!o.parse(inFile))
    {
        std::cerr<< "scene file parse error\n";
        return false;
    }

    // load meshes
    std::map<std::string, int> t;
    if(o.has<jsonxx::Object>("load_meshes")) {
        const auto& meshes = o.get<jsonxx::Object>("load_meshes");
        for(auto const& m: meshes.kv_map()) {
            std::string ref = m.first;
            std::string filename = m.second->get<jsonxx::String>(); 
            std::cout << ref << " " <<  filename << std::endl;
        }
    }
    else
        std::cout << "no meshes specified in scene\n";

    if(!o.has<jsonxx::Object>("world")) {
        std::cerr << "ERROR: no world in scene\n";
        return false;
    }

    const auto& world = o.get<jsonxx::Object>("world");

    if(!world.has<jsonxx::Array>("objects")) {
        std::cerr << "ERROR: no objects in scene\n";
        return false;
    }

    const auto& objects = world.get<jsonxx::Array>("objects");

    for(size_t i = 0; i < objects.size(); i++){
        handleObject(objects.get<jsonxx::Object>(i));
    }

    if(!world.has<jsonxx::Object>("lights")) {
        std::cerr << "ERROR: no lights in scene\n";
        return false;
    }

    const auto& lights = world.get<jsonxx::Object>("lights");

    return true;
}

inline bool setupScene(Scene& s)
{
    const int red_glass = 1, tiles = 2, reflective_blue=3;
    loadScene("scene/test1.scene", s);

    exit(0);
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

