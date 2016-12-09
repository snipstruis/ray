#pragma once

#include <cassert>
#include <fstream>
#include <vector>

// originally from https://danielbeard.wordpress.com/2011/06/06/image-saving-code-c/
void WriteTgaImage(unsigned int width, unsigned int height, ScreenBuffer const& screenBuffer) {
    assert(screenBuffer.size() == width * height);

	std::fstream o("image.tga", std::ios::out | std::ios::binary);

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
		o.put(screenBuffer[i].b*255);
		o.put(screenBuffer[i].g*255);
		o.put(screenBuffer[i].r*255);
	}   
}
