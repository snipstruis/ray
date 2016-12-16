
#include "interactive.h"
#include "batch.h"

#include "optionparser.h"

#include <string>
#include <iostream>

// default width/height
int width = 640;
int height = 640;

enum  optionIndex {UNKNOWN, HELP, INPUT_DIR, OUTPUT_DIR, SCENE, MODE};
const option::Descriptor usage[] =
{
	{UNKNOWN,    0,"" , "",           option::Arg::None, "USAGE: ray [options]\n\nOptions:"},
	{HELP,       0,"h", "help",       option::Arg::None, "  --help  -h\tPrint usage and exit."},
	{INPUT_DIR,  1,"i", "input-dir",  option::Arg::None, "  --input-dir, -i  \tinput directory"},
	{SCENE,      2,"s", "scene",      option::Arg::None, "  --scene, -s  \tscene file"},
	{OUTPUT_DIR, 3,"o", "output-dir", option::Arg::None, "  --output-dir, -o  \toutput directory"},
	{MODE,       0,"m", "mode",       option::Arg::None, "  --mode, -m  \trender mode [batch, interactive]"},

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

   	for (option::Option* opt = options[UNKNOWN]; opt; opt = opt->next())
     	std::cout << "Unknown option: " << opt->name << "\n";

   	for (int i = 0; i < parse.nonOptionsCount(); ++i)
     	std::cout << "Non-option #" << i << ": " << parse.nonOption(i) << "\n";

    std::cout << options[SCENE].count() << std::endl;
    std::cout << options[OUTPUT_DIR].count() << std::endl;

    std::string sceneFile = options[SCENE].arg;
    std::string outputDir = options[OUTPUT_DIR].arg;

    std::cout << "scenefile " << sceneFile << std::endl;
    std::cout << "outputDir " << outputDir << std::endl;

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
