#pragma once

#include "bvh_build_factory.h"
#include "output.h"
#include "render.h"
#include "timer.h"
#include "trace.h"
#include "utils.h"

int batchRender(Scene& s, std::string const& imgDir, int width, int height) {
    Timer t;
    Params p;

    ScreenBuffer screenBuffer;
    screenBuffer.resize(width * height);

    s.camera.width = width;
    s.camera.height = height;
    BVH* bvh = buildBVH(s, p.bvhMethod);

    std::cout << "starting batch render" << std::endl;

    renderFrame(s, *bvh, p, screenBuffer, 0);

    delete bvh;

    bool result = WriteTgaImage(imgDir, s.camera.width, s.camera.height, screenBuffer);

    std::cout << "render time " << t.sample() << " sec" << std::endl;

    return result ? 0 : -1;
}
