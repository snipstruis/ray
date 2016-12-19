#pragma once

#include "bvh_build.h"
#include "output.h"
#include "timer.h"
#include "trace.h"
#include "utils.h"

int batchRender(Scene& s, std::string const& imgDir, int width, int height) {
    Timer t;

    ScreenBuffer screenBuffer;
    screenBuffer.resize(width * height);

    s.camera.width = width;
    s.camera.height = height;

    BVH* bvh = buildBVH(s, BVHMethod_CENTROID_SAH);

    std::cout << "starting batch render" << std::endl;

    renderFrame(s, *bvh, screenBuffer, Mode::Default, 1.0f);

    delete bvh;

    bool result = WriteTgaImage(imgDir, s.camera.width, s.camera.height, screenBuffer);

    std::cout << "render time " << t.sample() << " sec" << std::endl;

    return result ? 0 : -1;
}
