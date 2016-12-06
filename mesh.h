#pragma once

#include "primitive.h"

#include "glm/vec3.hpp"

#include <map>
#include <string>
#include <vector>

struct Mesh {
    std::vector<Triangle> triangles;
};

typedef std::map<std::string, Mesh> MeshMap;

