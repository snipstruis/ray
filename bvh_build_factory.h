#pragma once

#include "bvh.h"
#include "bvh_build_stupid.h"
#include "bvh_build_centroid_sah.h"
#include "bvh_build_sbvh.h"
#include "bvh_diag.h"

#include "timer.h"

#include <iostream>

inline BVH* buildBVH(Scene& s, BVHMethod method) {
    Timer t;
    BVH* bvh = nullptr;

    switch(method) {
        case BVHMethod_STUPID:       bvh = buildStupidBVH(s);      break;
        case BVHMethod_CENTROID_SAH: bvh = buildCentroidSAHBVH(s); break;
        case BVHMethod_SBVH:         bvh = buildSBVH(s);           break;
        case __BVHMethod_MAX: assert(false); break; // shouldn't happen
    };

    assert(s.primitives.pos.size() == s.primitives.extra.size());

    std::cout << "world triangle count " << s.primitives.pos.size() << std::endl;
    std::cout << "BVH build time " << t.sample() << std::endl;

    sanityCheckBVH(*bvh, s.primitives.pos);
    dumpBVHStats(*bvh);

    return bvh;
}
