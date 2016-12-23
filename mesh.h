#pragma once

#include "primitive.h"

#include "glm/vec3.hpp"

#include <map>
#include <string>
#include <vector>

struct Mesh {
    TrianglePosSet pos;
    TriangleExtraSet extra;
};

typedef std::map<std::string, Mesh> MeshMap;

