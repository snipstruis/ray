#pragma once

#include <cassert>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>

// originally from https://danielbeard.wordpress.com/2011/06/06/image-saving-code-c/
void WriteNamedTgaImage(std::string const& fname, unsigned int width, unsigned int height, ScreenBuffer const& buf) {
    assert(buf.size() == width * height);

    std::cout << "screenshot - " << fname << std::endl;
	std::fstream o(fname, std::ios::out | std::ios::binary);

	// Write the header
	o.put(0);
   	o.put(0);
   	o.put(2);                       // uncompressed RGB 
   	o.put(0); 	o.put(0);
   	o.put(0); 	o.put(0);
   	o.put(0);
   	o.put(0); 	o.put(0);           // X origin 
   	o.put(0); 	o.put(0);           // y origin
   	o.put((width & 0x00FF));
   	o.put((width & 0xFF00) / 256);
   	o.put((height & 0x00FF));
   	o.put((height & 0xFF00) / 256);
   	o.put(24);                      // 24 bit bitmap
   	o.put(0);

	for (unsigned int i = 0; i < (height * width); i++) {
		o.put(buf[i].b * 255);
		o.put(buf[i].g * 255);
		o.put(buf[i].r * 255);
	}   

    if(!o.good())
        std::cout << "WARNING: error writing screenshot " << std::endl;
}

// generates a filename, and writes the image to dir
void WriteTgaImage(std::string const& dir, unsigned int width, unsigned int height, ScreenBuffer const& buf) {
    std::stringstream ss;

    if (dir.size() > 0)
        ss << dir << "/";

    ss << "ray-";

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    ss << std::put_time(std::localtime(&now), "%Y-%m-%d-%H.%M.%S");
    ss << ".tga";

    WriteNamedTgaImage(ss.str(), width, height, buf);
}