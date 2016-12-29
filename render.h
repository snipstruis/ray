#pragma once

#include "bvh.h"
#include "color.h"
#include "params.h"
#include "scene.h"
#include "trace.h"

// This file contains the render main loop, and all the pixel colouring code (ie the diagnostic visualisations)

Color value_to_color(float x){
    x*=5;if(x<1) {      return Color(  0,  0,  x);} // black -> blue
    else if(x<2) {x-=1; return Color(  0,  x,  1);} // blue  -> cyan
    else if(x<3) {x-=2; return Color(  0,  1,1-x);} // cyan  -> green
    else if(x<4) {x-=3; return Color(  x,  1,  0);} // green -> yellow
    else         {x-=4; return Color(  1,1-x,  0);} // yellow-> red
}

// Do a recursive ray trace (ie the default output)
struct StandardRenderer {
    static Color renderPixel(Ray const& r, Scene const& s, BVH const& bvh, Params const& p) {
        Color col = trace(r, bvh, s.primitives, s.lights, BLACK, p);
        return ColorClamp(col, 0.0f, 1.0f);
    }
};

// render surface normals
struct NormalRenderer {
    static Color renderPixel(Ray const& r, Scene const& s, BVH const& bvh, Params const& p) {
        auto hit = findClosestIntersectionBVH(bvh, s.primitives, r, p.traversalMode);

        if(hit.distance < INFINITY) {
            // we intersected. calc normal and convert to a col
            TrianglePos const& pos = s.primitives.pos[hit.triangle];
            TriangleExtra const& extra = s.primitives.extra[hit.triangle];
            auto fancy = FancyIntersect(hit.distance, pos, extra, r, p.smoothing);
            return Color((1.f+fancy.normal.x)/2.f, (1.f+fancy.normal.y)/2.f,  (1.f+fancy.normal.z)/2.f);
        } else {
            // no intersection
            return BLACK;
        }
    }
};

// do a recursive ray trace (ala StandardRenderer), but render the time taken as a color
struct PerformanceRenderer {
    static Color renderPixel(Ray const& r, Scene const& s, BVH const& bvh, Params const& p) {
        auto start = std::chrono::high_resolution_clock::now();
        // do the ray trace. we don't care about the result - just how long it took.
        trace(r, bvh, s.primitives, s.lights, BLACK, p);

        auto end = std::chrono::high_resolution_clock::now();
        auto frametime = std::chrono::duration_cast<std::chrono::duration<float,std::micro>>(end-start).count();

        return value_to_color(0.01 * p.visScale * frametime);
    }
};

struct BVHDiagRenderer {
    static Color renderPixel(Ray const& r, Scene const& s, BVH const& bvh, Params const& p) {
        DiagnosticCollector diag;

        auto hit = findClosestIntersectionBVH(bvh, s.primitives, r, diag, p.traversalMode);

        float intensity = 0.0f; 
        switch(p.visMode) {
            case VisMode::TrianglesChecked: intensity = diag.trianglesChecked; break;
            case VisMode::SplitsTraversed:  intensity = diag.splitsTraversed; break;
            case VisMode::LeafsChecked:     intensity = diag.leafsChecked; break;
            case VisMode::LeafNode:         intensity = diag.leafDepth; break;
            case VisMode::LeafBoxes:        
                if(diag.leafDepth > 0)                                
                    intensity = 1;; 
                break;
            case VisMode::NodeIndex:
                if(hit.distance < INFINITY)
                    intensity = diag.nodeIndex; 
                break;
            default:
                assert(false); // we shouldn't be called for other cases
        };
        
        return value_to_color(p.visScale * 0.001f * intensity);
    }
};

// main render loop
// assumes screenbuffer is big enough to handle the width*height pixels (per the camera)
template<class PixelRenderer>
inline void renderLoop(Scene const& s, BVH const& bvh, Params const& p, ScreenBuffer& screenBuffer) {
    unsigned int const width  = s.camera.width;
    unsigned int const height = s.camera.height;

    assert(screenBuffer.size() == width * height);

    #pragma omp parallel for schedule(auto) collapse(2)
    for (unsigned int y = 0; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            Ray r = s.camera.makeRay(x, y);
            unsigned int idx = (height-y-1) * width+ x;
            Color pixel = PixelRenderer::renderPixel(r, s, bvh, p);

            // it's the render's responsibility to check the pixel is in the legal range [0-1]
            assert(pixel.isLegal());

            screenBuffer[idx] = pixel;
        }
    }
}

// select the appropriate pixel renderer and launch the main loop
inline void renderFrame(Scene& s, BVH const& bvh, Params const& p, ScreenBuffer& screenBuffer){

    switch(p.visMode) {
    case VisMode::Default:
        renderLoop<StandardRenderer>(s, bvh, p, screenBuffer);
        break;
    case VisMode::Normal:
        renderLoop<NormalRenderer>(s, bvh, p, screenBuffer);
        break;
    case VisMode::Microseconds:
        renderLoop<PerformanceRenderer>(s, bvh, p, screenBuffer);
        break;
    default:
        renderLoop<BVHDiagRenderer>(s, bvh, p, screenBuffer);
        break;
    }
}

