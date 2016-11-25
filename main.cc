
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
            "Roaytroayzah res=(%d, %d) "
            "eye=(%0.3f, %0.3f, %0.3f) " 
            "yaw=%0.3f pitch=%0.3f roll=%0.3f "
            "fov=%0.3f ",
            s.camera.width, s.camera.height,
            s.camera.eye[0], s.camera.eye[1], s.camera.eye[2],
            rp.yaw, rp.pitch, rp.roll,
            glm::degrees(rp.fov)
            );

    SDL_SetWindowTitle(win, title);
}

int main(){
    SDL_Window *win = SDL_CreateWindow("Roaytroayzah (initialising)", 0, 0, 640, 480, 
                                       SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    SDL_GL_CreateContext(win);
    glClearColor(0,0,0,1);

    Scene s;

    unsigned prop = MAT_diffuse | MAT_shadow;
    s.primitives.emplace_back(new OutSphere(glm::vec3(0,0,10), 
                                            Material(Color(0.6,0.5,0.4),prop), 
                                            2));
    s.primitives.emplace_back(new Plane(glm::vec3(0,-1,0), 
                                        Material(Color(0.6,0.6,0.6),prop), 
                                        glm::vec3(0,1,0)));
    s.lights.emplace_back(glm::vec3(3,3,10), Color(0.6,0.6,0.6));

    RenderParams rp;

    Uint8 const * kbd = SDL_GetKeyboardState(NULL);
    while(true){
        // FIXME: save a bit of work by only doing this if camera's moved
        s.camera.buildCamera(rp);
        
        setWindowTitle(s, rp, win);
        
        // clear screen
        glClear(GL_COLOR_BUFFER_BIT);

        SDL_GL_GetDrawableSize(win,&s.camera.width,&s.camera.height);
        
        glViewport(0, 0, s.camera.width, s.camera.height);

        // draw pixels
        for (int y = 0; y < s.camera.height; y++) {
            for (int x = 0; x < s.camera.width; x++) {
                Ray r = s.camera.makeRay(x, y);

                // go forth and render..
                screenbuffer[(s.camera.height-y)*s.camera.width+x] = 
                    trace(r,s.primitives,s.lights,Color(0,0,0));
                //screenbuffer[y*w+x] = (Rgb){(float)y/h,(float)x/w,0.f};
            }
        }
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
        if(kbd[SDL_SCANCODE_W]) s.camera.eye[2] += 0.2;
        if(kbd[SDL_SCANCODE_S]) s.camera.eye[2] -= 0.2;
        if(kbd[SDL_SCANCODE_A]) s.camera.eye[0] -= 0.2;
        if(kbd[SDL_SCANCODE_D]) s.camera.eye[0] += 0.2;
    }
}
