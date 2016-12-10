#pragma once

#include "scene.h"
#include <string>

bool setupScene(Scene& s, std::string const& filename);

// it's a little odd to have this in loader, but it saves having to bring json.h anywhere else 
// (which improves compile time noticably outside loader.cc)
void printCamera(Camera const& c);


