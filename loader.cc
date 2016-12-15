#include "basics.h"
#include "debug_print.h"
#include "material.h"
#include "mesh.h"
#include "scene.h"

#include "json.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtx/io.hpp"
#include "glm/gtx/transform.hpp"
#include "tiny_obj_loader.h"

#include <fstream>
#include <string>
#include <vector>

using json = nlohmann::json;

glm::vec3 readXYZ(json const& o) {
    glm::vec3 res;
    res.x = o["x"];
    res.y = o["y"];
    res.z = o["z"];
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

glm::mat4 handleTransform(json const& o, bool rotateOnly) {
    glm::mat4 result; // is initialised to identity

    if(!rotateOnly && o.find("translate") != o.end()){
        auto const& translate = readXYZ(o["translate"]);
        result = glm::translate(result, translate);
    }

    if(o.find("rotate") != o.end()){
        auto const& rotate = readXYZ(o["rotate"]);
        result = glm::rotate(result, rotate[0], glm::vec3(0.1f, 0.0f, 0.0f));
        result = glm::rotate(result, rotate[1], glm::vec3(0.0f, 0.1f, 0.0f));
        result = glm::rotate(result, rotate[2], glm::vec3(0.0f, 0.0f, 0.1f));
    }

    if(!rotateOnly && o.find("scale") != o.end()){
        auto const& scale = readXYZ(o["scale"]);
        result = glm::scale(result, scale);
    }

    return result;
}

// all the verticies from tinyobj are in a flat array, in groups of 3.
// this function grabs 3 consecutive points and builds a vec3
// index is the tuple number - ie index 2 starts at array index 6
glm::vec3 makeVec3FromVerticies(tinyobj::attrib_t const& attrib, int index) {
    assert(index >= 0);
    int i = index * 3;
    return glm::vec3(attrib.vertices[i], attrib.vertices[i + 1], attrib.vertices[i + 2]);
}

glm::vec3 makeVec3FromNormals(tinyobj::attrib_t const& attrib, int index) {
    assert(index >= 0);
    int i = index * 3;
    return glm::vec3(attrib.normals[i], attrib.normals[i + 1], attrib.normals[i + 2]);
}

TrianglePosition BuildTrianglePos(tinyobj::attrib_t const& attrib, tinyobj::shape_t const& shape, int base) {
    return TrianglePosition(
        makeVec3FromVerticies(attrib, shape.mesh.indices[base].vertex_index),
        makeVec3FromVerticies(attrib, shape.mesh.indices[base + 1].vertex_index),
        makeVec3FromVerticies(attrib, shape.mesh.indices[base + 2].vertex_index));
}

// build the non-vertex parts of a triangle. requires a TrianglePos in case it needs to calculate its own normals
Triangle BuildTriangle(
        TrianglePosition const& pos,
        tinyobj::attrib_t const& attrib, 
        tinyobj::shape_t const& shape, 
        int base, 
        int globalMatID) {

    int i1 = shape.mesh.indices[base].normal_index;
    int i2 = shape.mesh.indices[base + 1].normal_index;
    int i3 = shape.mesh.indices[base + 2].normal_index;
    // if normals are missing in the input file, we'll get a -1 here 
    if(i1 < 0 || i2 < 0 || i3 < 0) {
        // we'll generate our own normals then.. With blackjack.. in fact, forget the normals.
        std::cout << "WARNING: missing normal" << std::endl;
        glm::vec3 n = glm::normalize(glm::cross(pos.v[1] - pos.v[0], pos.v[2] - pos.v[0]));
        return Triangle(n, n, n, globalMatID);
    } else {
        return Triangle(
            makeVec3FromNormals(attrib, i1),
            makeVec3FromNormals(attrib, i2),
            makeVec3FromNormals(attrib, i3),
            globalMatID
            );
    }
}

int createMaterial(Scene& s, tinyobj::material_t const& m){
    // create a new mat on the back of the existing array.
    s.primitives.materials.emplace_back(
            Color(m.diffuse[0], m.diffuse[1], m.diffuse[2]), // diffuse color
            Color(m.transmittance[0], m.transmittance[1], m.transmittance[2]), // transparency color
            (1.0f - m.dissolve), // transparency - note 1==opaque in the mat files.
            m.ior,  // refractive index
            -1,     // no checkerboard
            Color(m.specular[0], m.specular[1], m.specular[2]), //specular highlight color
            m.shininess);  // shininess

    // return the index of this newly created material.
    auto globalMatId = s.primitives.materials.size() - 1;
    std::cout << "created new material " << m.name << " @ " << globalMatId << std::endl;

    return globalMatId;
}

Mesh loadMesh(Scene& s, std::string const& filename){
    std::cout << "loading mesh " << filename << std::endl;

    std::string err;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str());
    if(!ret) {
        std::stringstream ss;
        ss << "ERROR loading mesh - " << err;
        throw std::runtime_error(ss.str());
    }
        
    std::cout << "material count " << materials.size() << std::endl;

    Mesh mesh;

    // we need to map local to global material number - track that here. 
    // this is local->global.
    // if a number is not in this map, it doesn't exist globally yet.
    std::map<int, int> matMap;

    for(auto const& shape : shapes) {
        // tinyobj should tesselate for us.
        assert(shape.mesh.indices.size() % 3 == 0);

        int ntriangles = shape.mesh.indices.size() / 3;

        for (int i = 0; i < ntriangles; i++) {
            int base = i * 3; 

            // map the local material id to the global number.
            int localMatID = shape.mesh.material_ids[i];
            int globalMatID;    

            // if this obj file has no materials, we'll get a -1 here
            if(localMatID < 0) {
                globalMatID = DEFAULT_MATERIAL;
            }
            else {
                // ok, triangle has a material...
                // have we created/mapped this material yet?
                auto it = matMap.find(localMatID);
                if(it == matMap.end()) {
                    // nope, need to create it.
                    globalMatID = createMaterial(s, materials[localMatID]);
                    matMap[localMatID] = globalMatID;
                }
                else {
                    globalMatID = it->second;
                }
            }
            // the array indicies of mesh.pos & mesh.triangles must line up - be careful here.
            assert(mesh.pos.size() == mesh.triangles.size());

            mesh.pos.push_back(BuildTrianglePos(attrib, shape, base));
            mesh.triangles.push_back(BuildTriangle(mesh.pos.back(), attrib, shape, base, globalMatID));
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
    return res;
}

// walk the whole mesh, copy-n-transform it into the world. 
// ie stamp it down, based on inputs from the scene.
void transformMeshIntoScene(Scene& s, 
        Mesh const& mesh, 
        glm::mat4x4 const& vTransform,
        glm::mat4x4 const& nTransform) {

    for(auto const& mt : mesh.triangles) {
        s.primitives.triangles.emplace_back(
            // normals
            glm::normalize(transformV3(mt.n[0], nTransform)), 
            glm::normalize(transformV3(mt.n[1], nTransform)), 
            glm::normalize(transformV3(mt.n[2], nTransform)), 
            mt.mat);
    }

    for(auto const& mt : mesh.pos){
        s.primitives.pos.emplace_back(
            transformV3(mt.v[0], vTransform), 
            transformV3(mt.v[1], vTransform), 
            transformV3(mt.v[2], vTransform));
    }
}

void handleMesh(Scene& s, MeshMap const& meshes, json const& o) {
    glm::mat4x4 vTransform, nTransform; // initialised to identity

    // is there a transform for this obj?
    if (o.find("transform") != o.end()) {
        vTransform = handleTransform(o["transform"], false);
        nTransform = handleTransform(o["transform"], true);
    }

    // find already loaded mesh
    std::string meshName = o["mesh_name"];
    auto it = meshes.find(meshName);
    if(it == meshes.end())
        throw std::runtime_error("unknown mesh");

    transformMeshIntoScene(s, it->second, vTransform, nTransform);
}

void handleObject(Scene& s, MeshMap const& meshes, json const& o) {

    const std::string kind = o["kind"];
    if(kind == "mesh"){
        handleMesh(s, meshes, o);
    }
    else if(kind == "sphere"){
        printf("warning: spheres no longer supported\n");
    }
    else if(kind == "plane"){
        printf("warning: planes no longer supported\n");
    }
    else {
        throw std::runtime_error("object kind unknown");
    }
}

void handleLight(Scene& s, json const& l) {
    const std::string kind = l["kind"];
    const glm::vec3 position = readXYZ(l["position"]);
    const Color color = readColor(l["color"]);

    if(kind == "point"){
        s.lights.pointLights.emplace_back(position, color);
    }
    else if(kind == "spot"){
        const glm::vec3 pointDirection = readPY(l["point_direction"]);
        float innerAngle = readAngle(l,"inner_angle");
        float outerAngle = readAngle(l,"outer_angle");
        FalloffKind falloff = readFalloffKind(l["falloff"]);

        s.lights.spotLights.emplace_back(position, pointDirection, color, falloff, innerAngle, outerAngle);
    }
    else {
        throw std::runtime_error("light kind unknown");
    }
}

void handleCamera(Scene& s, json const& c) {
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
            // check for dup key
            if(meshMap.find(it.key()) != meshMap.end()) {
                throw std::runtime_error("duplicate mesh key");
            }
            meshMap[it.key()] = loadMesh(s, it.value());
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
    buildFixedMaterials(s.primitives.materials);

    // sanity check - the fixed materials should now be created
    assert(s.primitives.materials.size() > 2);

    std::cout << "loading scene " << filename << std::endl;

    try{
        loadScene(s, filename);
    } catch (std::exception const& e) {
        std::cerr << "exception loading scene - " << e.what() << std::endl;
        return false;
    }

    return true;
}

// print camera in a form suitable for pasting in the scene file
void printCamera(Camera const& c) {
    json origin;
    origin["x"] = c.origin[0];
    origin["y"] = c.origin[1];
    origin["z"] = c.origin[2];

    json look_angle;
    look_angle["yaw"] = glm::degrees(c.yaw);
    look_angle["pitch"] = glm::degrees(c.pitch);

    json camera;
    camera["origin"] = origin;
    camera["look_angle"] = look_angle;
    camera["fov"] = glm::degrees(c.fov);

    std::cout << "\"camera\" : " << camera << std::endl;
}

