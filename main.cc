
#include "scene.h"
#include "camera.h"
#include "trace.h"
#include "debug_print.h"

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <cmath>
#include <chrono>
#include <vector>


void setWindowTitle(Scene const& s, SDL_Window *win, float frametime_ms)
{
    char title[1024];

    snprintf(title, sizeof(title),
            "Roaytroayzah %dx%d "
            "@ %2.3fms(%0.0f) "
            "eye=(%0.3f, %0.3f, %0.3f) " 
            "yaw=%0.0f pitch=%0.0f roll=%0.0f "
            "fov=%0.0f ",
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
    unsigned prop = MAT_checkered | MAT_diffuse | MAT_shadow;
    s.primitives.materials.emplace_back(Material(Color(0.6,0.5,0.4),
                                       prop|MAT_specular,0.7f));
    s.primitives.materials.emplace_back(Material(Color(0.6,0.6,0.6),
                                       prop));
    s.primitives.spheres.emplace_back(glm::vec3(0,0,10),0, 2.f);
    s.primitives.planes.emplace_back(Plane(glm::vec3(0,-1,0), 1, glm::vec3(0,1,0)));
    s.primitives.triangles.emplace_back(Triangle(glm::vec3(1,0,14), glm::vec3(-1,0,14), glm::vec3(0,2,14),1));
    s.lights.emplace_back(glm::vec3(3,3,10), Color(6,6,6));
    s.lights.emplace_back(glm::vec3(-4,2,8), Color(10,2,2));
    s.lights.emplace_back(glm::vec3(2,4,15), Color(2,10,2));
}

// process input
// returns true if app should quit
bool handleEvents(Scene& s)
{
    SDL_Event e;

    while(SDL_PollEvent(&e)) {
        switch(e.type)
        {
            case SDL_QUIT:
                return true;
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
                        return true;
                    case SDL_SCANCODE_R:  
                        s.camera.resetView();
                        break;
                    default:
                        break;
                }
                break;
        };
    }

    Uint8 const * kbd = SDL_GetKeyboardState(NULL);

    if(kbd[SDL_SCANCODE_S])
        s.camera.moveForward(-0.2);
    if(kbd[SDL_SCANCODE_W]) 
        s.camera.moveForward(0.2);
    if(kbd[SDL_SCANCODE_A])
        s.camera.moveRight(-0.2);
    if(kbd[SDL_SCANCODE_D])
        s.camera.moveRight(0.2);
    
    return false;
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

    std::vector<Color> screenBuffer; // will be allocated on first loop

    while(true){
        auto t_begin = std::chrono::high_resolution_clock::now();

        if(handleEvents(s))
            break;

        SDL_GL_GetDrawableSize(win, &s.camera.width, &s.camera.height);

        // FIXME: maybe save a bit of work by only doing this if camera's moved
        s.camera.buildCamera();

        // should be a no-op unless the window's resized
        screenBuffer.resize(s.camera.width * s.camera.height);

        glViewport(0, 0, s.camera.width, s.camera.height);

        renderFrame(s, screenBuffer);

        // blit to screen
        glDrawPixels(s.camera.width, s.camera.height, GL_RGB, GL_FLOAT, screenBuffer.data());

        auto t_end = std::chrono::high_resolution_clock::now();
        float frametime = std::chrono::duration_cast<std::chrono::duration<float,std::milli>>(t_end-t_begin).count();
        static float avg = 16;
        avg = 0.95*avg + 0.05*frametime;
        setWindowTitle(s, win, avg);
        SDL_GL_SwapWindow(win);
    }
}
