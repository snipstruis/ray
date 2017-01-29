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
            "%dx%d "
            "@ %2.3fms(%0.0ffps) "
            "%s "
            "bvh=%s "
            "(%0.3f, %0.3f, %0.3f) " 
            "fov=%0.0f "
            "color: %s",
            GetVisModeStr(p.visMode), 
            s.camera.width, s.camera.height,
            frametime*1000.0f, 1.0f/frametime,
            GetTraversalModeStr(p.traversalMode),
            GetBVHMethodStr(p.bvhMethod),
            s.camera.origin[0], s.camera.origin[1], s.camera.origin[2],
            glm::degrees(s.camera.fov),
            p.colorCorrection?"corrected":"uncorrected"
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
GuiAction handleEvents(Scene& s, float frameTime, Params& p, Uint8 const* kbd, bool& camera_dirty)
{
    SDL_Event e;
    float scale = frameTime;

    while(SDL_PollEvent(&e)) {
        switch(e.type)
        {
            case SDL_QUIT:
                return GA_QUIT;
            case SDL_MOUSEWHEEL:
                if(p.captureMouse){
                    s.camera.moveFov(glm::radians((float)-e.wheel.y * 10.f*scale));
                    camera_dirty=true;
                }
                break;
            case SDL_MOUSEMOTION:
                if(p.captureMouse){
                    float yaw = (((float)e.motion.xrel)/5) * 0.01;
                    float pitch = (((float)e.motion.yrel)/5) * 0.01;
                    s.camera.moveYawPitch(yaw, pitch);
                    camera_dirty=true;
                }
                break;
            case SDL_KEYDOWN:
                switch(e.key.keysym.scancode){
                    case SDL_SCANCODE_ESCAPE:  return GA_QUIT;
                    case SDL_SCANCODE_P: return GA_SCREENSHOT;
                    case SDL_SCANCODE_R: camera_dirty=true; s.camera.resetView(); break;
                    case SDL_SCANCODE_C: printCamera(s.camera); break;
                    case SDL_SCANCODE_M: camera_dirty=true; p.flipSmoothing(); break;
                    case SDL_SCANCODE_B: p.nextBvhMethod(); break;
                    case SDL_SCANCODE_T: p.flipTraversalMode(); break;
                    case SDL_SCANCODE_Q: p.captureMouse=!p.captureMouse; SDL_SetRelativeMouseMode(p.captureMouse ? SDL_TRUE : SDL_FALSE); break;
                    case SDL_SCANCODE_L: p.colorCorrection=!p.colorCorrection; break;
                    case SDL_SCANCODE_0: p.setVisMode(VisMode::Default); break;
                    case SDL_SCANCODE_1: p.setVisMode(VisMode::Microseconds); break;
                    case SDL_SCANCODE_2: p.setVisMode(VisMode::Normal); break;
                    case SDL_SCANCODE_3: p.setVisMode(VisMode::LeafDepth); break;
                    case SDL_SCANCODE_4: p.setVisMode(VisMode::TrianglesChecked); break;
                    case SDL_SCANCODE_5: p.setVisMode(VisMode::SplitsTraversed); break;
                    case SDL_SCANCODE_6: p.setVisMode(VisMode::LeavesChecked); break;
                    case SDL_SCANCODE_7: p.setVisMode(VisMode::NodeIndex); break;
                    case SDL_SCANCODE_8: p.setVisMode(VisMode::PathMicroseconds); break;
                    case SDL_SCANCODE_9: camera_dirty=true; p.setVisMode(VisMode::PathTrace); break;
                    default:
                        break;
                }
                break;
        };
    }


    if(kbd[SDL_SCANCODE_S]){camera_dirty=true; s.camera.moveForward(-2 * scale);}
    if(kbd[SDL_SCANCODE_W]){camera_dirty=true; s.camera.moveForward(2 * scale);}
    if(kbd[SDL_SCANCODE_A]){camera_dirty=true; s.camera.moveRight(-2 * scale);}
    if(kbd[SDL_SCANCODE_D]){camera_dirty=true; s.camera.moveRight(2 * scale);}
    if(kbd[SDL_SCANCODE_SPACE]){camera_dirty=true; s.camera.moveUp(2 * scale);}
    if(kbd[SDL_SCANCODE_LSHIFT]){camera_dirty=true; s.camera.moveUp(-2 * scale);}
    if(kbd[SDL_SCANCODE_COMMA]){camera_dirty=true; p.decVisScale();}
    if(kbd[SDL_SCANCODE_PERIOD]){camera_dirty=true; p.incVisScale();}
    
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
    p.setVisMode(VisMode::PathTrace);
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

    Uint8 const * kbd = SDL_GetKeyboardState(NULL);

    // will be sized on first loop
    ScreenBuffer screenBuffer; 
    ScreenBuffer clampedScreenBuffer;

    // for path-tracing
    bool camera_dirty = true;
    int passes = 0;

    AvgTimer frameTimer;
    int prev_width=0, prev_height=0;
    while(true){
        int width, height;
        SDL_GL_GetDrawableSize(win, &width, &height);
        if(prev_width!=width || prev_height!=height){
            //printf("window size change! %dx%d -> %dx%d\n",prev_width, prev_height, width, height);
            s.camera.width = width;
            s.camera.height= height;
            screenBuffer.resize(s.camera.width * s.camera.height);
            clampedScreenBuffer.resize(s.camera.width * s.camera.height);
            glViewport(0, 0, s.camera.width, s.camera.height);

            prev_width = width; prev_height = height;
            camera_dirty=true;
        }

        BVHMethod oldMethod = p.bvhMethod;
        GuiAction a = handleEvents(s, frameTimer.timer.lastDiff, p, kbd, camera_dirty);

        if (a==GA_QUIT)
            return 0;
        else if (a==GA_SCREENSHOT)
            WriteTgaImage(imgDir, s.camera.width, s.camera.height, clampedScreenBuffer);

        if(oldMethod != p.bvhMethod) {
            std::cout << "BVH method " << GetBVHMethodStr(oldMethod) << "->";
            std::cout << GetBVHMethodStr(p.bvhMethod) << std::endl;
            delete bvh;
            bvh = buildBVH(s, p.bvhMethod);
        }


        // if any params have changed, reset the average frametime
        if(p.dirty) {
            std::cout << "resetting average frametime" << std::endl;
            p.clearDirty();
            frameTimer.reset();
        }

        if(camera_dirty) {
            s.camera.buildCamera();
            passes = 0;
            camera_dirty = false;
        }

        renderFrame(s, *bvh, p, screenBuffer, passes++);
       
        // blit to screen
        if(p.colorCorrection){
            for(int i=0; i<screenBuffer.size(); i++){
                clampedScreenBuffer[i] = colorClamp(colorCorrect(screenBuffer[i]));
            }
        }else{
            for(int i=0; i<screenBuffer.size(); i++){
                clampedScreenBuffer[i] = colorClamp(screenBuffer[i]);
            }
        }
        glDrawPixels(s.camera.width, s.camera.height, GL_RGB, GL_FLOAT, clampedScreenBuffer.data());

        float frametimeAv = frameTimer.sample();
        if(frameTimer.timer.lastDiff > 1.0f)
            std::cout << "long render - frametime=" << frameTimer.timer.lastDiff << "s" << std::endl;

        setWindowTitle(s, win, frametimeAv, p);
        SDL_GL_SwapWindow(win);
    }

    return 0; // no error
}
