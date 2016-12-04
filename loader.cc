#include "basics.h"
#include "scene.h"
#include "loadObject.h"

#include "json.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtx/io.hpp"

#include <fstream>
#include <string>
#include <vector>

using json = nlohmann::json;

glm::vec3 readXYZ(json const& o) {
    glm::vec3 res;
    res[0] = o["x"];
    res[1] = o["y"];
    res[2] = o["z"];
    return res;
}

// read pitch/yaw, and convert to a normlised vector
glm::vec3 readPY(json const& o) {
    float pitch = o["pitch"];
    float yaw = o["yaw"];

    glm::vec3 res = glm::rotateX(glm::vec3(0,0,1), glm::radians(yaw));
    res = glm::rotateY(res, glm::radians(pitch));

    return res;
}

Color readColor(json const& o) {
    return Color(o["red"], o["green"], o["blue"]);
}

FalloffKind readFalloffKind(std::string const& s) {
    if(s == "log")
        return FK_LOG;
    else if (s == "linear")
        return FK_LINEAR;
    else
        throw std::runtime_error("unknown falloff kind");
}

glm::mat4 handleTransform(json const& o) {
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

void handleObject(Scene& s, json const& o) {
    std::cout << o << std::endl;
    const std::string kind = o["kind"];

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
        std::cout << "transformed " << transformed << std::endl;
        glm::vec3 center(transformed[0], transformed[1], transformed[2]);

        s.primitives.spheres.emplace_back(Sphere(center, 3, radius));

    }
    else if(kind == "plane"){
        glm::vec3 center = readXYZ(o["center"]);
        glm::vec3 normal = glm::normalize(readXYZ(o["normal"]));
        s.primitives.planes.emplace_back(Plane(center, 2, normal));
    }
    else {
        throw std::runtime_error("object kind unknown");
    }
}

void handleLight(Scene& s, json const& l) {
    std::cout << "LIGHT" << l << std::endl;
    const std::string kind = l["kind"];

    const glm::vec3 position = readXYZ(l["position"]);
    const Color color = readColor(l["color"]);

    if(kind == "point"){
        s.lights.pointLights.emplace_back(position, color);
    }
    else if(kind == "spot"){
        const glm::vec3 pointDirection = readPY(l["point_direction"]);
        float coneAngleDegrees = l["cone_angle"];
        float coneAngle = glm::radians(coneAngleDegrees);
        FalloffKind falloff = readFalloffKind(l["falloff"]);

        s.lights.spotLights.emplace_back(position, pointDirection, color, falloff, coneAngle);

        std::cout << "warning spot lights not supported yet "<< std::endl;
    }
    else {
        throw std::runtime_error("light kind unknown");
    }
}

bool loadScene(Scene& s, std::string const& filename)  {

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
        handleLight(s, light);
    }

    return true;
}

bool setupScene(Scene& s, std::string const& filename)
{
    const int red_glass = 1, tiles = 2, reflective_blue=3;

    try{
        loadScene(s, filename);
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
    s.primitives.materials.emplace_back(Color(0.f, 0.f, 0.f),
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
    return true;
}

