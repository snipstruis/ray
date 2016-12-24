
#include "batch.h"
#include "interactive.h"

#include <deque>
#include <string>
#include <iostream>

// default dimensions
int width = 640;
int height = 640;

int main(int argc, char* argv[]){
    std::deque<std::string> args;
    for(int i = 1; i < argc; i++)
        args.push_back(argv[i]);

    if (argc < 2 || argc > 4) {
        std::cout << "USAGE: " << argv[0] << "[-b] <scene file> [image output dir]\n";
        std::cout << "         -b  batch mode\n";
        return -1;
    }

	bool batch = false;
    if(args.front() == "-b") {
        std::cout << "batch mode\n";
        batch = true;
        args.pop_front();
    }

    std::string sceneFile = args.front();
    args.pop_front();

    std::string outputDir;

    if(!args.empty()) {
        outputDir = args.front();
        std::cout << "using output dir " << outputDir << "\n";
    }

    // setup scene first, so we can bail on error without flashing a window briefly (errors are stdout 
    // for now - maybe should be a dialog box in future).
    Scene s;
    if(!setupScene(s, sceneFile)) {
        std::cout << "ERROR: failed to setup scene, bailing" << std::endl;
        return -1;
    }

    if(s.primitives.pos.size() == 0) {
        std::cout << "ERROR: no triangles in scene" << std::endl;
        return -1;
    }

    if(batch)
         return batchRender(s, outputDir, width, height);
    else
         return interactiveLoop(s, outputDir, width, height);
}
