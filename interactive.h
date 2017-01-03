#pragma once

#include "bvh.h"
#include "bvh_build_factory.h"
#include "camera.h"
#include "loader.h"
#include "output.h"
#include "params.h"
#include "render.h"
#include "scene.h"
#include "timer.h"
#include "trace.h"

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#undef main

#include <cmath>
#include <vector>

// this file contains all machinery to operate interactive mode - ie whenever there is a visible window 
// note frametime is in seconds

void setWindowTitle(Scene const& s, SDL_Window *win, float frametime, Params const& p)
{
    char title[1024];

    snprintf(title, sizeof(title),
            "%s "
            "@ %2.3fms(%0.0f) "
            "%s "
            "bvh=%s "
            "o=(%0.3f, %0.3f, %0.3f) " 
            "y=%0.0f "
            "p=%0.0f "
            "f=%0.0f "
            "res %dx%d ",
            GetVisModeStr(p.visMode), 
            frametime*1000.0f, 1.0f/frametime,
            GetTraversalModeStr(p.traversalMode),
            GetBVHMethodStr(p.bvhMethod),
            s.camera.origin[0], s.camera.origin[1], s.camera.origin[2],
            glm::degrees(s.camera.yaw), glm::degrees(s.camera.pitch), 
            glm::degrees(s.camera.fov),
            s.camera.width, s.camera.height
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
GuiAction handleEvents(Scene& s, float frameTime, Params& p)
{
    SDL_Event e;
    float scale = frameTime;

    while(SDL_PollEvent(&e)) {
        switch(e.type)
        {
            case SDL_QUIT:
                return GA_QUIT;
            case SDL_MOUSEWHEEL:
                s.camera.moveFov(glm::radians((float)-e.wheel.y * scale));
                break;
            case SDL_MOUSEMOTION:
                {
                    // yes - it's "airplane" style at the moment - mouse down = view up.
                    // I'm going to get a cmdline working soon, make this an option
                    float yaw = (((float)e.motion.xrel)/5) * scale;
                    float pitch = -(((float)e.motion.yrel)/5) * scale;
                    s.camera.moveYawPitch(yaw, pitch);
                    break;
                }
            case SDL_KEYDOWN:
                switch(e.key.keysym.scancode){
                    case SDL_SCANCODE_ESCAPE:  return GA_QUIT;
                    case SDL_SCANCODE_P: return GA_SCREENSHOT;
                    case SDL_SCANCODE_R: s.camera.resetView(); break;
                    case SDL_SCANCODE_C: printCamera(s.camera); break;
                    case SDL_SCANCODE_M: p.flipSmoothing(); break;
                    case SDL_SCANCODE_B: p.nextBvhMethod(); break;
                    case SDL_SCANCODE_T: p.flipTraversalMode(); break;
                    case SDL_SCANCODE_0: p.setVisMode(VisMode::Default); break;
                    case SDL_SCANCODE_1: p.setVisMode(VisMode::Microseconds); break;
                    case SDL_SCANCODE_2: p.setVisMode(VisMode::Normal); break;
                    case SDL_SCANCODE_3: p.setVisMode(VisMode::LeafDepth); break;
                    case SDL_SCANCODE_4: p.setVisMode(VisMode::TrianglesChecked); break;
                    case SDL_SCANCODE_5: p.setVisMode(VisMode::SplitsTraversed); break;
                    case SDL_SCANCODE_6: p.setVisMode(VisMode::LeavesChecked); break;
                    case SDL_SCANCODE_7: p.setVisMode(VisMode::NodeIndex); break;
                    default:
                        break;
                }
                break;
        };
    }

    Uint8 const * kbd = SDL_GetKeyboardState(NULL);

    if(kbd[SDL_SCANCODE_S])
        s.camera.moveForward(-0.2f * scale);
    if(kbd[SDL_SCANCODE_W]) 
        s.camera.moveForward(0.2f * scale);
    if(kbd[SDL_SCANCODE_A])
        s.camera.moveRight(-0.2f * scale);
    if(kbd[SDL_SCANCODE_D])
        s.camera.moveRight(0.2f * scale);
    if(kbd[SDL_SCANCODE_COMMA])
        p.decVisScale();
    if(kbd[SDL_SCANCODE_PERIOD])
        p.incVisScale();
    
    return GA_NONE;
}

// main loop when in interactive mode
// @s: pre-populated scene
// @imgDir: location to write screenshots
//
int interactiveLoop(Scene& s, std::string const& imgDir, int width, int height) {
    // first thing's first, create the BVH
    // do this before opening the window to ease debugging

    Params p;
    BVH* bvh = buildBVH(s, p.bvhMethod);

    // max depth is a good starting value for vis scale - at least for bvh stats.. 
    // maybe consider a different scale value for other outputs like microseconds
    p.autoSetVisScale((float)bvh->maxDepth);

    SDL_Window *win = SDL_CreateWindow("Roaytroayzah (initialising)", 50, 50, width, height, 
                                       SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    SDL_GL_CreateContext(win);

    // clear screen (probably not strictly nescessary)
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);

    // switch on relative mouse mode - hides the cursor, and kinda makes things... relative.
    SDL_SetRelativeMouseMode(SDL_TRUE);

    ScreenBuffer screenBuffer; // will be sized on first loop

    AvgTimer frameTimer;
    while(true){
        SDL_GL_GetDrawableSize(win, &s.camera.width, &s.camera.height);

        BVHMethod oldMethod = p.bvhMethod;
        GuiAction a = handleEvents(s, frameTimer.timer.lastDiff, p);

        if (a==GA_QUIT)
            break;
        else if (a==GA_SCREENSHOT)
            WriteTgaImage(imgDir, s.camera.width, s.camera.height, screenBuffer);

        if(oldMethod != p.bvhMethod) {
            std::cout << "BVH method " << GetBVHMethodStr(oldMethod) << "->";
            std::cout << GetBVHMethodStr(p.bvhMethod) << std::endl;
            delete bvh;
            bvh = buildBVH(s, p.bvhMethod);
        }

        // if any params have changed, reset the average
        if(p.dirty) {
            std::cout << "resetting average frametime" << std::endl;
            p.clearDirty();
            frameTimer.reset();
        }

        // FIXME: maybe save a bit of work by only doing this if camera's moved
        s.camera.buildCamera();
        screenBuffer.resize(s.camera.width * s.camera.height);

        glViewport(0, 0, s.camera.width, s.camera.height);

        renderFrame(s, *bvh, p, screenBuffer);
       
        // blit to screen
        glDrawPixels(s.camera.width, s.camera.height, GL_RGB, GL_FLOAT, screenBuffer.data());

        float frametimeAv = frameTimer.sample();
        if(frameTimer.timer.lastDiff > 1.0f)
            std::cout << "long render - frametime=" << frameTimer.timer.lastDiff << "s" << std::endl;

        setWindowTitle(s, win, frametimeAv, p);
        SDL_GL_SwapWindow(win);
    }

    return 0; // no error
}
