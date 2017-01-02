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

#ifdef ENABLE_BOOST_IOSTREAMS
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#endif

using json = nlohmann::json;

// concepts loaded from TinyObj
struct LoadedObject {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
};

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

glm::mat4 handleTransform(json const& o) {
    glm::mat4 result; // is initialised to identity

    if(o.find("translate") != o.end()){
        auto const& translate = readXYZ(o["translate"]);
        result = glm::translate(result, translate);
    }

    if(o.find("rotate") != o.end()){
        auto const& rotate = readXYZ(o["rotate"]);
        result = glm::rotate(result, rotate[0], glm::vec3(0.1f, 0.0f, 0.0f));
        result = glm::rotate(result, rotate[1], glm::vec3(0.0f, 0.1f, 0.0f));
        result = glm::rotate(result, rotate[2], glm::vec3(0.0f, 0.0f, 0.1f));
    }

    // for now, we don't allow a zero scaling value. this could be legal if you wanted to
    // flatten something into 2d.. but this produces rubbish normals. needs further thought 
    // (eg is there any reason to scale normals, or should we go back to having a separate normal transform.
    // they really only need to be rotated, as we normalize them regardless).
    if(o.find("scale") != o.end()){
        auto const& scale = readXYZ(o["scale"]);
        if(!(scale.x > 0.0f) || !(scale.y > 0.0f) || !(scale.z > 0.0f))
            throw std::runtime_error("can't have zero scaling value");

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

TrianglePos BuildTrianglePos(tinyobj::attrib_t const& attrib, tinyobj::shape_t const& shape, int base) {
    return TrianglePos(
        makeVec3FromVerticies(attrib, shape.mesh.indices[base].vertex_index),
        makeVec3FromVerticies(attrib, shape.mesh.indices[base + 1].vertex_index),
        makeVec3FromVerticies(attrib, shape.mesh.indices[base + 2].vertex_index));
}

// try to make a normal from the file input. 
// requires a TrianglePos in case it needs to calculate its own normals
glm::vec3 makeVec3FromNormals(tinyobj::attrib_t const& attrib, int index, TrianglePos const& pos) {
    // if the input file doesn't have a defined normal, we'll get a -1 here.
    if(index >= 0) {
        int i = index * 3;

        // danger.. normals from file are not nescessarily normalised!
        const glm::vec3 base(attrib.normals[i], attrib.normals[i+1], attrib.normals[i+2]);
        const glm::vec3 n = glm::normalize(base);

        // even though we've called normalize(), normals might still be bad (eg zero length)
        // thus only return the normal here if it's good...
        if(glm::isNormalized(n, EPSILON))
            return n;
    }

    // if we've fallen through to here, 
    // we'll generate our own normals then.. With blackjack.. in fact, forget the normals.
    std::cout << "WARNING: calculating our own normal" << std::endl;
    return glm::normalize(glm::cross(pos.v[1] - pos.v[0], pos.v[2] - pos.v[0]));
}

// build the non-vertex parts of a triangle. 
// requires a TrianglePos in case it needs to calculate its own normals
TriangleExtra BuildTriangleExtra(
        TrianglePos const& pos,
        tinyobj::attrib_t const& attrib, 
        tinyobj::shape_t const& shape, 
        int base, 
        int globalMatID) {

    int i0 = shape.mesh.indices[base].normal_index;
    int i1 = shape.mesh.indices[base + 1].normal_index;
    int i2 = shape.mesh.indices[base + 2].normal_index;

    auto n0 = makeVec3FromNormals(attrib, i0, pos);
    auto n1 = makeVec3FromNormals(attrib, i1, pos);
    auto n2 = makeVec3FromNormals(attrib, i2, pos);

    // stupidity checking.. sigh
    assert(glm::isNormalized(n0, EPSILON));
    assert(glm::isNormalized(n1, EPSILON));
    assert(glm::isNormalized(n2, EPSILON));

    return TriangleExtra(n0, n1, n2, globalMatID);
}

int createMaterial(Scene& s, tinyobj::material_t const& m){
    // create a new mat on the back of the existing array.
    s.primitives.materials.emplace_back(
            Color(m.diffuse[0], m.diffuse[1], m.diffuse[2]), // diffuse color
            Color(m.transmittance[0], m.transmittance[1], m.transmittance[2]), // transparency color
            (1.0f - m.dissolve), // transparency - note 1==opaque in the mat files.
            m.ior,  // refractive index
            -1,     // no checkerboard
            Color(m.specular[0], m.specular[1], m.specular[2]), // specular highlight color
            m.shininess);  // shininess

    // return the index of this newly created material.
    auto globalMatId = s.primitives.materials.size() - 1;
    std::cout << "created new material " << m.name << " @ " << globalMatId << std::endl;

    return globalMatId;
}

template <class StreamType>
void loadObject(std::string const& inputDir, StreamType& stream, LoadedObject& obj) {

    std::string err;
    std::string matDir = "./";
    
    // need to ensure the material dir has a trailing slash, as tinyobj seems to naively smash the
    // dir and filename together.
    if(inputDir.size() > 0) {
        std::stringstream ss;
        ss << inputDir << "/";
        matDir = ss.str();
    }

    tinyobj::MaterialFileReader m(matDir);

    bool ret = tinyobj::LoadObj(&obj.attrib, &obj.shapes, &obj.materials, &err, &stream, &m);
    if(!ret) {
        std::stringstream ss;
        ss << "ERROR loading mesh - " << err;
        throw std::runtime_error(ss.str());
    }
}

void setupStream(std::string const& inputDir, std::string const& filename, LoadedObject& obj) {
    std::stringstream ss;
    if(inputDir.size() > 0)
        ss << inputDir << "/";

    ss << filename;
    std::cout << "loading mesh " << ss.str() << std::endl;

#ifdef ENABLE_BOOST_IOSTREAMS
    // try gzip stream
    {
        std::ifstream f(ss.str(), std::ios_base::in | std::ios_base::binary);
        boost::iostreams::filtering_stream<boost::iostreams::input> in;
        in.push(boost::iostreams::gzip_decompressor());
        in.push(f);

        // need to peek at least one byte to trigger any errors
        in.peek();

        if(in.good()) {
            std::cout << "... loading gzip'd stream" << std::endl;
            loadObject(inputDir, in, obj);
            return;
        }
    }
#endif

    // try uncompressed stream
    {
        std::ifstream f(ss.str(), std::ios_base::in);

        if(f.good()) {
            loadObject(inputDir, f, obj);
            return;
        }
    }

    throw std::runtime_error("couldn't setup stream");
}

Mesh loadMesh(std::string const& inputDir, std::string const& filename, Scene& s){

    LoadedObject obj;
    setupStream(inputDir, filename, obj);
        
    std::cout << "material count " << obj.materials.size() << std::endl;

    Mesh mesh;

    // we need to map local to global material number - track that here. 
    // this is local->global.
    // if a number is not in this map, it doesn't exist globally yet.
    std::map<int, int> matMap;

    for(auto const& shape : obj.shapes) {
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
            } else {
                // ok, triangle has a material...
                // have we created/mapped this material yet?
                auto it = matMap.find(localMatID);
                if(it == matMap.end()) {
                    // nope, need to create it.
                    globalMatID = createMaterial(s, obj.materials[localMatID]);
                    matMap[localMatID] = globalMatID;
                } else {
                    globalMatID = it->second;
                }
            }
            // the array indicies of mesh.pos & mesh.triangles must line up - be careful here.
            assert(mesh.pos.size() == mesh.extra.size());

            mesh.pos.push_back(BuildTrianglePos(obj.attrib, shape, base));
            mesh.extra.push_back(BuildTriangleExtra(mesh.pos.back(), obj.attrib, shape, base, globalMatID));
        }
    }

    // triangles are split in two - must have exactly the same count in both arrays
    assert(mesh.pos.size() == mesh.extra.size());
    std::cout << "load done, triangles = " << mesh.pos.size() << std::endl;
    return mesh;
}

// apply the matrix transform to v
// @v: vec3 src vector
// @transform: transform to apply
// @w: 4th coordinate for transform - ie the linear transform part.
//      usually, use 1.0f for verticies, and 0.0 for normals
glm::vec3 transformV3(glm::vec3 const& v, glm::mat4x4 const& transform, float w) {
    glm::vec4 a(v, w);
    glm::vec4 b = transform * a;
    return glm::vec3(b); // grab (only) the first 3 coords of b
}

// walk the whole mesh, copy-n-transform it into the world. 
// ie stamp it down, based on inputs from the scene.
void transformMeshIntoScene(Scene& s, Mesh const& mesh, glm::mat4x4 const& transform) {
    assert(mesh.extra.size() == mesh.pos.size());
    s.primitives.pos.reserve(mesh.pos.size());
    s.primitives.extra.reserve(mesh.extra.size());

    int cullCount = 0;
    for(unsigned int i = 0; i < mesh.pos.size(); i++){
        TrianglePos const& origPos = mesh.pos[i];

        TrianglePos pos(
            transformV3(origPos.v[0], transform, 1.0f), 
            transformV3(origPos.v[1], transform, 1.0f), 
            transformV3(origPos.v[2], transform, 1.0f));

        if(!pos.hasArea()) {
            cullCount++;
            continue;
        }

        s.primitives.pos.push_back(pos);

        TriangleExtra const& origExtra = mesh.extra[i];

        s.primitives.extra.emplace_back(
            glm::normalize(transformV3(origExtra.n[0], transform, 0.0f)), 
            glm::normalize(transformV3(origExtra.n[1], transform, 0.0f)), 
            glm::normalize(transformV3(origExtra.n[2], transform, 0.0f)), 
            origExtra.mat);
    }
    if(cullCount > 0) {
        std::cout << "Culled " << cullCount << " triangles" << std::endl;
    }
}

void handleMesh(Scene& s, MeshMap const& meshes, json const& o) {
    glm::mat4x4 transform; // initialised to identity

    // is there a transform for this obj?
    if (o.find("transform") != o.end()) {
        transform = handleTransform(o["transform"]);
    }

    // find already loaded mesh
    std::string meshName = o["mesh_name"];
    auto it = meshes.find(meshName);
    if(it == meshes.end())
        throw std::runtime_error("unknown mesh");

    transformMeshIntoScene(s, it->second, transform);
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

bool loadScene(std::string const& inputDir, std::string const& filename, Scene& scene) {
    std::stringstream ss;
    if(inputDir.size() > 0)
        ss << inputDir << "/";

    ss << filename;
    std::cout << "loading scene " << ss.str() << std::endl;
    std::ifstream inFile(ss.str());

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
            meshMap[it.key()] = loadMesh(inputDir, it.value(), scene);
        }
    }
    else {
        std::cout << "no meshes specified in scene\n";
    }

    std::cout << meshMap.size() << " meshes loaded " << std::endl;

    auto const& world = o["world"];
    auto const& objects = world["objects"];

    for (auto const& object : objects) {
        handleObject(scene, meshMap, object);
    }

    auto const& lights = world["lights"];
    for (auto const& light: lights) {
        handleLight(scene, light);
    }

    if(o.find("camera") != o.end()) {
        handleCamera(scene, o["camera"]);
    }

    return true;
}

bool setupScene(std::string const& inputDir, std::string const& filename, Scene& scene)
{
    buildFixedMaterials(scene.primitives.materials);

    // sanity check - the fixed materials should now be created
    assert(scene.primitives.materials.size() > 2);

    try{
        loadScene(inputDir, filename, scene);
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

