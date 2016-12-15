#pragma once

#include "trace.h"
#include "utils.h"

int batchRender(Scene& s, std::string const& imgDir, int width, int height) {
    ScreenBuffer screenBuffer;
    screenBuffer.resize(width * height);

    s.camera.width = width;
    s.camera.height = height;

    BVH* bvh = buildBVH(s, BVHMethod_MEDIAN);

    std::cout << "starting batch render" << std::endl;

    renderFrame(s, *bvh, screenBuffer, Mode::Default, 1.0f);

    delete bvh;

    return WriteTgaImage(imgDir, s.camera.width, s.camera.height, screenBuffer) ? 0 : -1;
}
