
#include "scene.h"
#include "camera.h"
#include "trace.h"
#include "debug_print.h"

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <iostream>

struct Rgb{ float r,g,b; };

// fixed size for now
const int WIDTH = 1920;
const int HEIGHT = 1080;

Rgb screenbuffer[WIDTH*HEIGHT];

void renderImage(Scene& s)
{
    std::cout << "starting render" << std::endl;

    for (int y = 0; y < s.camera.height; y++) {
        for (int x = 0; x < s.camera.width; x++) {
            Ray r = s.camera.makeRay(x, y);

            // go forth and render..
            trace(r,s.primitives,s.lights);
        }
    }
}

int main(){
    SDL_Window *win = SDL_CreateWindow("Roaytroayzah", 0, 0, 640, 480, 
                                       SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    SDL_GL_CreateContext(win);
    glClearColor(0,0,0,1);

    Scene s;
   // s.camera.width = WIDTH;
   // s.camera.height = HEIGHT;
    s.camera.width = 3;
    s.camera.height = 5;

    renderImage(s);
    
    while(true){
        // clear screen
        glClear(GL_COLOR_BUFFER_BIT);

        int w,h; 
        SDL_GL_GetDrawableSize(win,&w,&h);
        
        glViewport(0, 0, w, h);

        // draw pixels
        for(int y=0; y<h; y++){
            for(int x=0; x<w; x++){
                screenbuffer[y*w+x] = (Rgb){(float)y/h,(float)x/w,0.f};
            }
        }

        // blit to screen
        glDrawPixels(w,h,GL_RGB,GL_FLOAT,&screenbuffer);
        SDL_GL_SwapWindow(win);

        // handle events
        SDL_Event e;
        SDL_WaitEvent(&e);
        do{ if(e.type==SDL_QUIT) 
                return 0; 
        }while(SDL_PollEvent(&e));
    }
}
