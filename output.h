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
}

// generates a filename, and writes the image to dir
void WriteTgaImage(std::string const& dir, unsigned int width, unsigned int height, ScreenBuffer const& buf) {
    std::stringstream ss;
    ss << dir << "/";

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    ss << std::put_time(std::localtime(&now), "%Y-%m-%d-%H.%M.%S");
    ss << ".tga";

    WriteNamedTgaImage(ss.str(), width, height, buf);
    std::cout << "wrote image " << ss.str() << std::endl;
}
