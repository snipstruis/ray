#pragma once

enum class VisMode {
    Default,
    Microseconds,
    Normal,
    NodeIndex,
    SplitsTraversed,
    TrianglesChecked,
    LeafsChecked,
    LeafNode,
    LeafBoxes
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
        case VisMode::LeafBoxes: return "leaf boxes";
    }
};

// parameters to current render.
struct Params {
    Params() : 
        visMode(VisMode::Default),
        smoothing(true)
    {}

    VisMode visMode;
    bool smoothing;
};

