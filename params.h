#pragma once

enum class VisMode {
    Default,
    Microseconds,
    Normal,
    NodeIndex,
    SplitsTraversed,
    TrianglesChecked,
    LeafsChecked,
    LeafNode
};

const char* GetVisModeStr(VisMode m) {
    switch(m) {
        case VisMode::Default: return "wooosh";
        case VisMode::Microseconds: return "frametime";
        case VisMode::Normal: return "normal";
        case VisMode::NodeIndex: return "node index";
        case VisMode::SplitsTraversed: return "splits traversed";
        case VisMode::TrianglesChecked: return "triangles checked";
        case VisMode::LeafsChecked: return "leafs checked";
        case VisMode::LeafNode: return "leaf depth";
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
//    STUPID,
    CENTROID_SAH,
    SBVH,
    _MAX
};

const char* GetBVHMethodStr(BVHMethod m) {
    switch(m) {
 //       case BVHMethod::STUPID: return "STUPID";
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
        smoothing(true)
    {}

    void flipSmoothing() {
        smoothing = !smoothing;
    }

    void flipTraversalMode() {
        if(traversalMode == TraversalMode::Ordered)
            traversalMode = TraversalMode::Unordered;
        else
            traversalMode = TraversalMode::Ordered;
    }

    void nextBvhMethod() {
        bvhMethod = (BVHMethod)(((int)(bvhMethod) + 1) % (int)BVHMethod::_MAX);
    }

    VisMode visMode;
    float visScale;
	TraversalMode traversalMode;
    BVHMethod bvhMethod;
    bool smoothing;
};

