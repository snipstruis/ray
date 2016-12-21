#pragma once

#include "bvh.h"
#include "color.h"
#include "scene.h"
#include "trace.h"

enum class Mode {
    Default,
    Microseconds,
    Normal,
    NodeIndex,
    SplitsTraversed,
    TrianglesChecked,
    LeafsChecked,
    LeafNode
};

char const * const modestr[] = {
    "GOTTA GO FAST",
    "frametime",
    "normal",
    "node index",
    "splits traversed",
    "triangles checked",
    "leafs checked",
    "leaf depth"
};

Color value_to_color(float x){
    x*=5;if(x<1) {      return Color(  0,  0,  x);} // black -> blue
    else if(x<2) {x-=1; return Color(  0,  x,  1);} // blue  -> cyan
    else if(x<3) {x-=2; return Color(  0,  1,1-x);} // cyan  -> green
    else if(x<4) {x-=3; return Color(  x,  1,  0);} // green -> yellow
    else         {x-=4; return Color(  1,1-x,  0);} // yellow-> red
}

// main render starting loop
// assumes screenbuffer is big enough to handle the width*height pixels (per the camera)
inline void renderFrame(Scene& s, BVH& bvh, std::vector<Color>& screenBuffer, Mode mode, float vis_scale){
    // draw pixels
    int const width  = s.camera.width;
    int const height = s.camera.height;
    switch(mode){
    case Mode::Default:
        #pragma omp parallel for schedule(auto) collapse(2)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Ray r = s.camera.makeRay(x, y);
                int idx = (height-y-1) * width+ x;
                auto col = trace(r, bvh, s.primitives, s.lights, Color(0,0,0));
                screenBuffer[idx] = ColorClamp(col, 0.0f, 1.0f);
            }
        }
        break;
    case Mode::Microseconds:
        #pragma omp parallel for schedule(auto) collapse(2)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                auto start = std::chrono::high_resolution_clock::now();
                Ray r = s.camera.makeRay(x, y);
                int idx = (height-y-1) * width+ x;
                screenBuffer[idx] = trace(r, bvh, s.primitives, s.lights, Color(0,0,0));
                auto end = std::chrono::high_resolution_clock::now();
                auto frametime = 
                    std::chrono::duration_cast<std::chrono::duration<float,std::micro>>(end-start).count();
                screenBuffer[idx] = value_to_color(0.01*vis_scale*frametime);
            }
        }
        break;
    case Mode::Normal:
        #pragma omp parallel for schedule(auto) collapse(2)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Ray r = s.camera.makeRay(x,y);
                int idx = (height-y-1) * width+ x;
                auto hit = findClosestIntersectionBVH(bvh, s.primitives, r);
                if(hit.distance < INFINITY) {
                    Triangle const& tri = s.primitives.triangles[hit.triangle];
                    TrianglePosition const& pos = s.primitives.pos[hit.triangle];
                    auto fancy = FancyIntersect(hit.distance, pos, tri, r);
                    screenBuffer[idx] = Color((1.f+fancy.normal.x)/2.f, 
                                              (1.f+fancy.normal.y)/2.f, 
                                              (1.f+fancy.normal.z)/2.f);
                }
                else {
                    screenBuffer[idx] = BLACK;
                }
            }
        }
        break;
    default:
        #pragma omp parallel for schedule(auto) collapse(2)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Ray r = s.camera.makeRay(x, y);
                int idx = (height-y-1) * width+ x;
                auto diag = BVHIntersectDiag();
                auto hit  = findClosestIntersectionBVH_DIAG(bvh, s.primitives, r, &diag);
                float intensity = 
                    mode==Mode::TrianglesChecked? vis_scale*0.001f*diag.trianglesChecked
                  : mode==Mode::SplitsTraversed?  vis_scale*0.001f*diag.splitsTraversed
                  : mode==Mode::LeafsChecked?     vis_scale*0.001f*diag.leafsChecked
                  : mode==Mode::NodeIndex&&hit.distance!=INFINITY?vis_scale*0.001f*hit.nodeIndex 
                  : mode==Mode::LeafNode?         vis_scale*0.001f*hit.leafDepth: 0;
                screenBuffer[idx] = value_to_color(intensity);
            }
        }
        break;
    }
}

