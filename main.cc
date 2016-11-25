
#include "scene.h"
#include "camera.h"
#include "trace.h"
#include "debug_print.h"

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <iostream>
#include <memory>
#include <chrono>

// fixed size for now
const int WIDTH = 1920;
const int HEIGHT = 1080;

Color screenbuffer[WIDTH*HEIGHT];

void setWindowTitle(Scene const& s, SDL_Window *win, float frametime_ms)
{
    char title[1024];

    snprintf(title, sizeof(title),
            "Roaytroayzah %dx%d "
            "@ %2.3fms(%0.0f) "
            "eye=(%0.3f, %0.3f, %0.3f) " 
            "yaw=%0.3f pitch=%0.3f roll=%0.3f "
            "fov=%0.3f ",
            s.camera.width, s.camera.height,
            frametime_ms, 1000.f/frametime_ms,
            s.camera.eye[0], s.camera.eye[1], s.camera.eye[2],
            glm::degrees(s.camera.yaw), glm::degrees(s.camera.pitch), glm::degrees(s.camera.roll),
            glm::degrees(s.camera.fov)
            );

    SDL_SetWindowTitle(win, title);
}

void setupScene(Scene& s)
{
    unsigned prop = MAT_diffuse | MAT_shadow;
    s.primitives.emplace_back(new OutSphere(glm::vec3(0,0,10), 
                                            Material(Color(0.6,0.5,0.4),prop), 
                                            2));
    s.primitives.emplace_back(new Plane(glm::vec3(0,-1,0), 
                                        Material(Color(0.6,0.6,0.6),prop), 
                                        glm::vec3(0,1,0)));
    s.lights.emplace_back(glm::vec3(3,3,10), Color(0.6,0.6,0.6));
}

void renderFrame(Scene& s){
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
}

int main(){
    SDL_Window *win = SDL_CreateWindow("Roaytroayzah (initialising)", 0, 0, 640, 640, 
                                       SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    SDL_GL_CreateContext(win);

    // clear screen (probably not strictly nescessary)
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);

    Scene s;
    setupScene(s);

    // switch on relative mouse mode - hides the cursor, and kinda makes things... relative.
    SDL_SetRelativeMouseMode(SDL_TRUE);
    Uint8 const * kbd = SDL_GetKeyboardState(NULL);

    while(true){
        auto t_begin = std::chrono::high_resolution_clock::now();
        // handle events
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            switch(e.type)
            {
                case SDL_QUIT:
                    return 0;
                case SDL_MOUSEWHEEL:
                    s.camera.moveFov(glm::radians((float)-e.wheel.y));
                    break;
                case SDL_MOUSEMOTION:
                    // yes - it's "airplane" style at the moment - mouse down = view up.
                    // I'm going to get a cmdline working soon, make this an option
                    s.camera.moveYawPitch(
                        glm::radians(((float)e.motion.xrel)/5), -glm::radians(((float)e.motion.yrel)/5));
                    break;
                case SDL_KEYDOWN:
                    switch(e.key.keysym.scancode){
                        case SDL_SCANCODE_ESCAPE:  
                            return 0;
                        case SDL_SCANCODE_R:  
                            s.camera.resetView();
                            break;
                        default:
                            break;
                    }
                    break;
            };
        }

        if(kbd[SDL_SCANCODE_S])
            s.camera.moveForward(-0.2);
        if(kbd[SDL_SCANCODE_W]) 
            s.camera.moveForward(0.2);
        if(kbd[SDL_SCANCODE_A])
            s.camera.moveRight(-0.2);
        if(kbd[SDL_SCANCODE_D])
            s.camera.moveRight(0.2);
        // FIXME: maybe save a bit of work by only doing this if camera's moved
        s.camera.buildCamera();
        

        SDL_GL_GetDrawableSize(win, &s.camera.width, &s.camera.height);
        
        glViewport(0, 0, s.camera.width, s.camera.height);

        renderFrame(s);

        // blit to screen
        glDrawPixels(s.camera.width,s.camera.height,GL_RGB,GL_FLOAT,&screenbuffer);

        auto t_end = std::chrono::high_resolution_clock::now();
        float frametime = std::chrono::duration_cast<std::chrono::duration<float,std::milli>>(t_end-t_begin).count();
        static float avg = 16;
        avg = 0.95*avg + 0.05*frametime;
        setWindowTitle(s, win, avg);
        SDL_GL_SwapWindow(win);
        

    }
}
