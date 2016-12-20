#pragma once

#include "bvh.h"
#include "bvh_build_stupid.h"
#include "bvh_build_centroid_bvh.h"
#include "bvh_diag.h"

#include "timer.h"

#include <iostream>

inline BVH* buildBVH(Scene& s, BVHMethod method) {
    Timer t;
    BVH* bvh = nullptr;

    switch(method) {
        case BVHMethod_STUPID:       bvh = buildStupidBVH(s); break;
        case BVHMethod_CENTROID_SAH: bvh = buildCentroidSAHBVH(s); break;
        //case BVHMethod_SAH:    bvh = buildSAHBVH(s);    break;
        case __BVHMethod_MAX: assert(false); break; // shouldn't happen
    };

    std::cout << "bvh node count " << bvh->nextFree << std::endl;
    std::cout << "triangle count " << s.primitives.triangles.size() << std::endl;
    std::cout << "root AABB " << bvh->root().bounds << std::endl;
    std::cout << "BVH build time " << t.sample() << std::endl;

    sanityCheckBVH(*bvh, s.primitives.pos);

    return bvh;
}
