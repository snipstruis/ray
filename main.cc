
#include "scene.h"
#include "camera.h"
#include "loader.h"
#include "trace.h"
#include "debug_print.h"

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#undef main

#include <cmath>
#include <chrono>
#include <vector>

void DoScreenshot(int x, int y, std::vector<Color>& screenBuffer) {

}

void setWindowTitle(Scene const& s, SDL_Window *win, float frametime_ms)
{
    char title[1024];

    snprintf(title, sizeof(title),
            "Roaytroayzah %dx%d "
            "@ %2.3fms(%0.0f) "
            "o=(%0.3f, %0.3f, %0.3f) " 
            "y=%0.0f "
            "p=%0.0f "
            "f=%0.0f ",
            s.camera.width, s.camera.height,
            frametime_ms, 1000.f/frametime_ms,
            s.camera.origin[0], s.camera.origin[1], s.camera.origin[2],
            glm::degrees(s.camera.yaw), glm::degrees(s.camera.pitch), 
            glm::degrees(s.camera.fov)
            );

    SDL_SetWindowTitle(win, title);
}

enum GuiAction {
    GA_NONE,
    GA_QUIT,
    GA_SCREENSHOT
};
// process input
// returns action to be performed
GuiAction handleEvents(Scene& s, Mode *vis)
{
    SDL_Event e;

    while(SDL_PollEvent(&e)) {
        switch(e.type)
        {
            case SDL_QUIT:
                return GA_QUIT;
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
                    case SDL_SCANCODE_ESCAPE:  return GA_QUIT;
                    case SDL_SCANCODE_P:  return GA_SCREENSHOT;
                    case SDL_SCANCODE_R:  s.camera.resetView(); break;
                    case SDL_SCANCODE_0: *vis = Mode::Default; break;
                    case SDL_SCANCODE_1: *vis = Mode::Microseconds; break;
                    default:
                        break;
                }
                break;
        };
    }

    Uint8 const * kbd = SDL_GetKeyboardState(NULL);

    if(kbd[SDL_SCANCODE_S])
        s.camera.moveForward(-0.2f);
    if(kbd[SDL_SCANCODE_W]) 
        s.camera.moveForward(0.2f);
    if(kbd[SDL_SCANCODE_A])
        s.camera.moveRight(-0.2f);
    if(kbd[SDL_SCANCODE_D])
        s.camera.moveRight(0.2f);
    
    return GA_NONE;
}

int main(int argc, char* argv[]){
    if(argc != 2) {
        std::cerr << "usage " << argv[0] << " <scenefile>" << std::endl;
        return -1;
    }

    // setup scene first, so we can bail on error without flashing a window briefly (errors are stdout 
    // for now - maybe should be a dialog box in future).
    Scene s;
    if(!setupScene(s, argv[1])) {
        std::cout << "failed to setup scene, bailing" << std::endl;
        return -1;
    }

    SDL_Window *win = SDL_CreateWindow("Roaytroayzah (initialising)", 100, 100, 640, 640, 
                                       SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    SDL_GL_CreateContext(win);

    // clear screen (probably not strictly nescessary)
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);

    // switch on relative mouse mode - hides the cursor, and kinda makes things... relative.
    SDL_SetRelativeMouseMode(SDL_TRUE);

    std::vector<Color> screenBuffer; // will be allocated on first loop

    auto now = std::chrono::high_resolution_clock::now();

    Mode mode=Mode::Default;
    while(true){
        SDL_GL_GetDrawableSize(win, &s.camera.width, &s.camera.height);

        GuiAction a = handleEvents(s,&mode);
        if (a==GA_QUIT)
            break;
        else if (a==GA_SCREENSHOT)
            DoScreenshot(s.camera.width, s.camera.height, screenBuffer);

        // FIXME: maybe save a bit of work by only doing this if camera's moved
        s.camera.buildCamera();

        // should be a no-op unless the window's resized
        screenBuffer.resize(s.camera.width * s.camera.height);

        glViewport(0, 0, s.camera.width, s.camera.height);

        renderFrame(s, screenBuffer, mode);
       
        // blit to screen
        glDrawPixels(s.camera.width, s.camera.height, GL_RGB, GL_FLOAT, screenBuffer.data());

        auto last = now;
        now = std::chrono::high_resolution_clock::now();
        float frametime = 
                std::chrono::duration_cast<std::chrono::duration<float,std::milli>>(now - last).count();

        if(frametime > 1000)
            std::cout << "long render - frametime=" << frametime/1000.0f << "s" << std::endl;

        static float avg = frametime;
        avg = 0.95f*avg + 0.05f*frametime;

        setWindowTitle(s, win, avg);
        SDL_GL_SwapWindow(win);
    }
}
