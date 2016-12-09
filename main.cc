
#include "interactive.h"


int main(int argc, char* argv[]){
    if(argc != 2) {
        std::cerr << "usage " << argv[0] << " <scenefile>" << std::endl;
        return -1;
    }

    // setup scene first, so we can bail on error without flashing a window briefly (errors are stdout 
    // for now - maybe should be a dialog box in future).
    Scene s;
    if(!setupScene(s, argv[1])) {
        std::cout << "failed to setup scene, bailing" << std::endl;
        return -1;
    }

    return interactiveLoop(s);
}
