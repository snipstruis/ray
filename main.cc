
#include "interactive.h"

#include <boost/program_options.hpp>

int main(int argc, char* argv[]){
    namespace po = boost::program_options;

    std::string sceneFile, imgDir;

	po::options_description desc("options");
	desc.add_options()
   		("help", "produce help message")
    	("scene", po::value<std::string>(&sceneFile)->required(), "scene file name")
    	("image-dir", po::value<std::string>()->required(), "image output dir");

	po::variables_map vm;

    try {
	    po::store(po::parse_command_line(argc, argv, desc), vm);
	    po::notify(vm);    
    } catch (po::error const& e) {
    	std::cout << "error parsing commandline: " << e.what()<< "\n";
        return -1;
    }

	if (vm.count("help")) {
    	std::cout << desc << "\n";
	    return 0;
	}
return 0;


std::string s2;
    try {
    s2 = vm["scene"].as<std::string>();
    } catch (boost::bad_any_cast const& e) {
        std::cout << e.what() << std::endl;
    }
    std::cout << s2; 
    std::cout <<"\n";
    return 0;

    // setup scene first, so we can bail on error without flashing a window briefly (errors are stdout 
    // for now - maybe should be a dialog box in future).
    Scene s;
    if(!setupScene(s, vm["scene"].as<std::string>())) {
        std::cout << "failed to setup scene, bailing" << std::endl;
        return -1;
    }

    return interactiveLoop(s, "/Users/nick");
}
