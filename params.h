#pragma once

enum class VisMode {
    Default,
    Microseconds,
    PathMicroseconds,
    Normal,
    NodeIndex,
    SplitsTraversed,
    TrianglesChecked,
    LeavesChecked,
    LeafDepth,
    PathTrace
};

const char* GetVisModeStr(VisMode m) {
    switch(m) {
        case VisMode::Default: return "wooosh";
        case VisMode::Microseconds: return "whitted frametime";
        case VisMode::PathMicroseconds: return "path frametime";
        case VisMode::Normal: return "normal";
        case VisMode::NodeIndex: return "node index";
        case VisMode::SplitsTraversed: return "splits traversed";
        case VisMode::TrianglesChecked: return "triangles checked";
        case VisMode::LeavesChecked: return "leaves checked";
        case VisMode::LeafDepth: return "leaf depth";
        case VisMode::PathTrace: return "path trace";
    }

	return ""; // silence msvc warn
};

enum class TraversalMode{
    Unordered,
    Ordered
};

const char* GetTraversalModeStr(TraversalMode m) {
    switch (m) {
        case TraversalMode::Unordered: return "unordered";
        case TraversalMode::Ordered: return "ordered";
    }
	return ""; // silence msvc warn
}

enum class BVHMethod {
    SBVH,
    CENTROID_SAH,
    STUPID,
    _MAX
};

const char* GetBVHMethodStr(BVHMethod m) {
    switch(m) {
        case BVHMethod::STUPID: return "STUPID";
        case BVHMethod::CENTROID_SAH: return "SAH";
        case BVHMethod::SBVH: return "SBVH";
        case BVHMethod::_MAX: return "shouldn't happen";
    };
	return ""; // silence msvc warn
}

// parameters to current render.
struct Params {
    Params() : 
        visMode(VisMode::Default),
        visScale(1.0f),
		traversalMode(TraversalMode::Ordered),
        bvhMethod(BVHMethod::SBVH),
        smoothing(true),
        dirty(true),
        visScaleSetManually(false),
        captureMouse(true),
        colorCorrection(true)
    {}

    void flipSmoothing() {
        smoothing = !smoothing;
        dirty = true;
    }

    void flipTraversalMode() {
        if(traversalMode == TraversalMode::Ordered)
            traversalMode = TraversalMode::Unordered;
        else
            traversalMode = TraversalMode::Ordered;
        dirty = true;
    }

    void nextBvhMethod() {
        bvhMethod = (BVHMethod)(((int)(bvhMethod) + 1) % (int)BVHMethod::_MAX);
        dirty = true;
    }

    void setVisMode(VisMode m) {
        visMode = m;
        dirty = true;
    }

    // if the user's not automatically overridden it, auto set the scale
    void autoSetVisScale(float val) {
        if(!visScaleSetManually) {
            visScale = val;
            std::cout << "auto set vis scale to " << val << std::endl;
        }
    }
    void incVisScale() {
        visScale *= 1.1f;
        visScaleSetManually = true;
    }

    void decVisScale() {
        visScale *= 0.9f;
        visScaleSetManually = true;
    }

    void clearDirty() {
        dirty = false;
    }

    VisMode visMode;
    float visScale;
	TraversalMode traversalMode;
    BVHMethod bvhMethod;
    bool smoothing;
    bool captureMouse;
    bool dirty; // has something changed recently?
    bool visScaleSetManually; // has the user explicitly adjusted vis scale? (ie pressed . or ,) ? 
    bool colorCorrection;
};

