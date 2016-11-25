
#include "scene.h"
#include "camera.h"
#include "trace.h"
#include "debug_print.h"

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <iostream>
#include <memory>

// fixed size for now
const int WIDTH = 1920;
const int HEIGHT = 1080;

Color screenbuffer[WIDTH*HEIGHT];

void setWindowTitle(Scene const& s, RenderParams const& rp, SDL_Window *win)
{
    char title[1024];

    snprintf(title, sizeof(title) - 1,
            "Roaytroayzah eye=(%0.3f, %0.3f, %0.3f) " 
            "top_left=(%0.3f, %0.3f, %0.3f) "
            "u=(%0.3f, %0.3f, %0.3f) "
            "v=(%0.3f, %0.3f, %0.3f) ",
            s.camera.eye[0], s.camera.eye[1], s.camera.eye[2],
            s.camera.top_left[0], s.camera.top_left[1], s.camera.top_left[2],
            s.camera.u[0], s.camera.u[1], s.camera.u[2],
            s.camera.v[0], s.camera.v[1], s.camera.v[2]
            );

    SDL_SetWindowTitle(win, title);
}

int main(){
    SDL_Window *win = SDL_CreateWindow("Roaytroayzah (initialising)", 0, 0, 640, 480, 
                                       SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    SDL_GL_CreateContext(win);
    glClearColor(0,0,0,1);

    Scene s;
    RenderParams rp;
    
    s.primitives.emplace_back(new OutSphere(glm::vec3(0,0,10), Material(Color(0.6,0.5,0.4)), 2));

    while(true){
        // FIXME: save a bit of work by only doing this if camera's moved
        s.camera.buildCamera(rp);
        
        setWindowTitle(s, rp, win);
        
        // clear screen
        glClear(GL_COLOR_BUFFER_BIT);

        SDL_GL_GetDrawableSize(win,&s.camera.width,&s.camera.height);
        
        glViewport(0, 0, s.camera.width, s.camera.height);

        // blit to screen
        glDrawPixels(s.camera.width,s.camera.height,GL_RGB,GL_FLOAT,&screenbuffer);
        SDL_GL_SwapWindow(win);

        // handle events
        SDL_Event e;
        while(SDL_PollEvent(&e)) {

            switch(e.type)
            {
                case SDL_QUIT:
                    return 0;
            };
        }

    }
}
