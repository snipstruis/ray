#pragma once

#include "primitive.h"

#include "glm/vec3.hpp"

#include <map>
#include <string>
#include <vector>

struct Mesh {
    TriangleSet triangles;
    TrianglePosSet pos;
};

typedef std::map<std::string, Mesh> MeshMap;

