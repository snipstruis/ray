#pragma once

#include <cassert>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>

// originally from https://danielbeard.wordpress.com/2011/06/06/image-saving-code-c/
bool WriteNamedTgaImage(std::string const& fname, unsigned int w, unsigned int h, ScreenBuffer const& buf) {
    assert(buf.size() == w * h);

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
   	o.put((w & 0x00FF));
   	o.put((w & 0xFF00) / 256);
   	o.put((h & 0x00FF));
   	o.put((h & 0xFF00) / 256);
   	o.put(24);                      // 24 bit bitmap
   	o.put(0);

	for (unsigned int i = 0; i < (h * w); i++) {
		o.put(buf[i].b * 255);
		o.put(buf[i].g * 255);
		o.put(buf[i].r * 255);
	}   

    if(!o.good()) {
        std::cout << "WARNING: error writing screenshot " << std::endl;
        return false;
    }
    return true;
}

// generates a filename, and writes the image to dir
bool WriteTgaImage(std::string const& dir, unsigned int width, unsigned int height, ScreenBuffer const& buf) {
    std::stringstream ss;

    if (dir.size() > 0)
        ss << dir << "/";

    ss << "ray-";

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    ss << std::put_time(std::localtime(&now), "%Y-%m-%d-%H.%M.%S");
    ss << ".tga";

    return WriteNamedTgaImage(ss.str(), width, height, buf);
}
