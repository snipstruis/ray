
#include "interactive.h"
#include "batch.h"

#include "optionparser.h"

#include <string>
#include <iostream>

// default width/height
int width = 640;
int height = 640;

enum  optionIndex { UNKNOWN, HELP, PLUS };
const option::Descriptor usage[] =
{
	{UNKNOWN, 0,"" , ""    ,option::Arg::None, "USAGE: example [options]\n\n"
                                               "Options:" },
	{HELP,    0,"" , "help",option::Arg::None, "  --help  \tPrint usage and exit." },
	{PLUS,    0,"p", "plus",option::Arg::None, "  --plus, -p  \tIncrement count." },
	{UNKNOWN, 0,"" ,  ""   ,option::Arg::None, "\nExamples:\n"
                                               "  example --unknown -- --this_is_no_option\n"
                                               "  example -unk --plus -ppp file1 file2\n" },
	{0,0,0,0,0,0}
};

int main(int argc, char* argv[]){

   	argc -= (argc > 0); 
	argv += (argc > 0);
   	option::Stats stats(usage, argc, argv);
   	option::Option options[stats.options_max], buffer[stats.buffer_max];
   	option::Parser parse(usage, argc, argv, options, buffer);

   	if(parse.error())
    	return -1;

   	if(options[HELP] || argc == 0) {
    	option::printUsage(std::cout, usage);
     	return 0;
   	}

   	std::cout << "--plus count: " <<
     	options[PLUS].count() << "\n";

   	for (option::Option* opt = options[UNKNOWN]; opt; opt = opt->next())
     	std::cout << "Unknown option: " << opt->name << "\n";

   	for (int i = 0; i < parse.nonOptionsCount(); ++i)
     	std::cout << "Non-option #" << i << ": " << parse.nonOption(i) << "\n";



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

    bool batch = false;
    if(batch)
        return batchRender(s, imgDir, width, height);
    else
        return interactiveLoop(s, imgDir, width, height);
}
