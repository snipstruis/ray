#pragma once

#include "bvh.h"
#include "bvh_build_stupid.h"
#include "bvh_build_centroid_sah.h"
#include "bvh_build_sbvh.h"
#include "bvh_diag.h"

#include "params.h"
#include "timer.h"

#include <iostream>

inline BVH* buildBVH(Scene& s, BVHMethod method) {
    // empty scenes should already be caught
    assert(s.primitives.pos.size() > 0);
    assert(s.primitives.pos.size() == s.primitives.extra.size());

    Timer t;
    BVH* bvh = nullptr;

    switch(method) {
        case BVHMethod::STUPID:       bvh = buildStupidBVH(s);      break;
        case BVHMethod::CENTROID_SAH: bvh = buildCentroidSAHBVH(s); break;
        case BVHMethod::SBVH:         bvh = buildSBVH(s);           break;
        case BVHMethod::_MAX: assert(false); break; // shouldn't happen
    };

    std::cout << "world triangle count " << s.primitives.pos.size() << std::endl;
    std::cout << "BVH build time " << t.sample() << std::endl;

    sanityCheckBVH(*bvh, s.primitives.pos);
    dumpBVHStats(*bvh, s.primitives.pos);

    return bvh;
}
