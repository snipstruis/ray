
#include "interactive.h"

int main(int argc, char* argv[]){

    if (argc < 2 || argc > 3) {
        std::cout << "USAGE: " << argv[0] << " <scene file> [image output dir]\n";
        return -1;
    }

    std::string sceneFile = argv[1];
    std::string imgDir;

    if (argc == 3) {
        imgDir = argv[2];
        std::cout << "using img output dir " << imgDir << "\n";
    }

    // setup scene first, so we can bail on error without flashing a window briefly (errors are stdout 
    // for now - maybe should be a dialog box in future).
    Scene s;
    if(!setupScene(s, sceneFile)) {
        std::cout << "failed to setup scene, bailing" << std::endl;
        return -1;
    }

    return interactiveLoop(s, imgDir);
}
