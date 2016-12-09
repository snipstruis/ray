
#include "interactive.h"

#include <boost/program_options.hpp>

int main(int argc, char* argv[]){
    if(argc != 2) {
        std::cerr << "usage " << argv[0] << " <scenefile>" << std::endl;
        return -1;
    }

    namespace po = boost::program_options;

	po::options_description desc("Allowed options");
	desc.add_options()
   		("help", "produce help message")
    	("compression", po::value<int>(), "set compression level");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);    

	if (vm.count("help")) {
    	std::cout << desc << "\n";
	    return 1;
	}

	if (vm.count("compression")) {
    	std::cout << "Compression level was set to "  << vm["compression"].as<int>() << ".\n";
	} 
	else {
    	std::cout << "Compression level was not set.\n";
	}

    // setup scene first, so we can bail on error without flashing a window briefly (errors are stdout 
    // for now - maybe should be a dialog box in future).
    Scene s;
    if(!setupScene(s, argv[1])) {
        std::cout << "failed to setup scene, bailing" << std::endl;
        return -1;
    }

    return interactiveLoop(s, "/Users/nick");
}
