#include "basics.h"
#include "mesh.h"
#include "scene.h"

#include "json.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtx/io.hpp"
#include "tiny_obj_loader.h"

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

// read angle as degrees, convert to radians
float readAngle(json const& o, const char* name) {
    float value = o[name];
    return glm::radians(value);
}

// read pitch/yaw, and convert to a normlised vector
glm::vec3 readPY(json const& o) {
    glm::vec3 res = glm::rotateX(glm::vec3(0,0,1), readAngle(o, "yaw"));
    res = glm::rotateY(res, readAngle(o, "pitch"));
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

// all the verticies from tinyobj are in a flat array, in groups of 3.
// this function grabs 3 consecutive points and builds a vec3
// index is the tuple number - ie index 2 starts at array index 6
glm::vec3 makeVec3FromVerticies(tinyobj::attrib_t const& attrib, int index) {
    int i = index * 3;
    glm::vec3 result(attrib.vertices[i], attrib.vertices[i + 1], attrib.vertices[i + 2]);
    return result;
}

Mesh loadMesh(std::string const& filename){
    std::string err;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::cout << "loading " << filename << std::endl;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str());
    if(!ret) {
        std::stringstream ss;
        ss << "ERROR loading object - " << err;
        throw std::runtime_error(ss.str());
    }
        
    std::cout << "material count " << materials.size() << std::endl;
    Mesh mesh;


    for(auto const& shape : shapes) {
        // tinyobj should tesselate for us.
        assert(shape.mesh.indices.size() % 3 == 0);

        int ntriangles = shape.mesh.indices.size() / 3;

        for (int i = 0; i < ntriangles; i++) {
            int base = i * 3; 

            MeshTriangle t(
                makeVec3FromVerticies(attrib, shape.mesh.indices[base].vertex_index),
                makeVec3FromVerticies(attrib, shape.mesh.indices[base + 1].vertex_index),
                makeVec3FromVerticies(attrib, shape.mesh.indices[base + 2].vertex_index));

            mesh.triangles.emplace_back(t);
        }
    }

    std::cout << "load done, triangles = " << mesh.triangles.size() << std::endl;
    return mesh;
}

// apply the matrix transform to v
glm::vec3 transformV3(glm::vec3 v, glm::mat4x4 transform) {
    // FIXME: ugly, remove temp objects
    glm::vec4 a(v[0], v[1], v[2], 1);
    glm::vec4 b = transform * a;
    glm::vec3 res(b[0], b[1], b[2]);

    std::cout << " TTT " << v << " -> " << res << std::endl;

    return res;
}

void transformMeshIntoScene(Scene& s, Mesh const& mesh, glm::mat4x4 const& transform) {
    for(auto const& mt : mesh.triangles) {
        Triangle t(
            transformV3(mt.v1, transform), 
            transformV3(mt.v2, transform), 
            transformV3(mt.v3, transform), 
            3);
        s.primitives.triangles.emplace_back(t);
    }
}

void handleObject(Scene& s, MeshMap const& meshes, json const& o) {

    std::cout << o << std::endl;
    const std::string kind = o["kind"];

    glm::mat4x4 transform; // initialised to identity
    // is there a transform for this obj?
    if (o.find("transform") != o.end()) {
        transform = handleTransform(o["transform"]);
        std::cout << "got transform " << transform << std::endl;
    }

    if(kind == "mesh"){
        // find already loaded mesh
        std::string meshName = o["mesh_name"];
        auto it = meshes.find(meshName);
        if(it == meshes.end())
            throw std::runtime_error("unknown mesh");

        transformMeshIntoScene(s, it->second, transform);
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

void handleCamera(Scene& s, json const& c) {
    std::cout << "got camera "<< c << std::endl;
    s.camera.startingOrigin = readXYZ(c["origin"]);

    auto const& lookAngle = c["look_angle"];
    s.camera.startingYaw = readAngle(lookAngle, "yaw");
    s.camera.startingPitch = readAngle(lookAngle, "pitch");

    s.camera.startingFov = readAngle(c, "fov");
    s.camera.resetView();
}

bool loadScene(Scene& s, std::string const& filename)  {

    std::ifstream inFile(filename);
    json o;
    inFile >> o;

    MeshMap meshMap;

    // load meshes
    if(o.find("load_meshes") != o.end()) {
        for(auto it = o["load_meshes"].begin(); it != o["load_meshes"].end(); ++it) {
            std::cout << it.key() << " " << it.value() << std::endl;
            
            // check for dup key
            if(meshMap.find(it.key()) != meshMap.end()) {
                throw std::runtime_error("duplicate mesh key");
            }

            meshMap[it.key()] = loadMesh(it.value());
        }
    }
    else {
        std::cout << "no meshes specified in scene\n";
    }

    std::cout << meshMap.size() << " meshes loaded " << std::endl;

    auto const& world = o["world"];
    auto const& objects = world["objects"];

    for (auto const& object : objects) {
        handleObject(s, meshMap, object);
    }

    auto const& lights = world["lights"];
    for (auto const& light: lights) {
        handleLight(s, light);
    }

    if(o.find("camera") != o.end()) {
        handleCamera(s, o["camera"]);
    }

    return true;
}

bool setupScene(Scene& s, std::string const& filename)
{
    const int reflective_blue=3;

    std::cout << "loading scene " << filename << std::endl;

    try{
        loadScene(s, filename);
    } catch (std::exception const& e) {
        std::cerr << "exception loading scene - " << e.what() << std::endl;
        return false;
    }

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
                                        0.8f, // diffuse 
                                        0.0f, // reflective
                                        0.0f, // transparency
                                        1.f,  // refractive index
                                        -1,   // no checkerboard
                                        0.8,  // specular highlight
                                        32.f);  // shinyness
    return true;
}

