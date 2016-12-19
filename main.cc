
#include "batch.h"
#include "interactive.h"

#include <string>
#include <iostream>

// default dimensions
int width = 640;
int height = 640;

int main(int argc, char* argv[]){

    if (argc < 2 || argc > 3) {
        std::cout << "USAGE: " << argv[0] << " <scene file> [image output dir]\n";
        return -1;
    }

    std::string sceneFile = argv[1];
    std::string outputDir;

    if (argc == 3) {
        outputDir = argv[2];
        std::cout << "using output dir " << outputDir << "\n";
    }

    // setup scene first, so we can bail on error without flashing a window briefly (errors are stdout 
    // for now - maybe should be a dialog box in future).
    Scene s;
    if(!setupScene(s, sceneFile)) {
        std::cout << "failed to setup scene, bailing" << std::endl;
        return -1;
    }

	bool batch = false;
    if(batch)
         return batchRender(s, outputDir, width, height);
    else
         return interactiveLoop(s, outputDir, width, height);
}
