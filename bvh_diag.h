#pragma once

#include "bvh.h"
#include "primitive.h"

#include <iostream>
#include <vector>

struct BVHStatsPerLeaf {
    BVHStatsPerLeaf(unsigned int _depth, unsigned int _triCount) : depth(_depth), triCount(_triCount) {}

    unsigned int depth;
    unsigned int triCount;
};

struct BVHStatsTotal {
    BVHStatsTotal() : totalNodes(0) {}

    unsigned int totalNodes;
    std::vector<BVHStatsPerLeaf> perLeaf;
};

void dumpBVHStatsRecurse(BVH const& bvh, BVHNode const& node, int depth, BVHStatsTotal& stats) {
    auto const& left = bvh.getNode(node.leftIndex());
    auto const& right = bvh.getNode(node.rightIndex());

    stats.totalNodes++;

    if(node.isLeaf()) {
        stats.perLeaf.emplace_back(depth, node.count);
    } else {
        dumpBVHStatsRecurse(bvh, left, depth + 1, stats);
        dumpBVHStatsRecurse(bvh, right, depth + 1, stats);
    }
}

void dumpBVHStats(BVH const& bvh){
    BVHStatsTotal stats;
    dumpBVHStatsRecurse(bvh, bvh.root(), 0, stats);

    unsigned int maxTri = 0;
    unsigned int minTri = std::numeric_limits<unsigned int>::max();
    unsigned int sumTri = 0;

    unsigned int maxDepth = 0;
    unsigned int minDepth = std::numeric_limits<unsigned int>::max();
    unsigned int sumDepth = 0;

    for(auto const& p : stats.perLeaf) {
        minTri = std::min(minTri, p.triCount);
        maxTri = std::max(maxTri, p.triCount);
        sumTri += p.triCount;

        minDepth = std::min(minDepth, p.depth);
        maxDepth = std::max(maxDepth, p.depth);
        sumDepth += p.depth;
    }

    float avgDepth = sumDepth / ((float)stats.perLeaf.size());
    float avgTri = sumTri / ((float)stats.perLeaf.size());

    std::cout << "=========================================================================================\n";
    std::cout << "BVH Stats\n";
    std::cout << "root AABB " << bvh.root().bounds << "\n";
    std::cout << "Total nodes " << bvh.nodes.size() << " total leaves " << stats.perLeaf.size();
    std::cout << " Total tri indicies " << bvh.indicies.size() << "\n";
    std::cout << "Per leaf: min tri   " << minTri << " max Tri " << maxTri << " avgTri " << avgTri << "\n";
    std::cout << "          min depth " << minDepth << " max Depth " << maxDepth<< " avgDepth " << avgDepth<< "\n";


    std::cout << "=========================================================================================\n";
}

// recursively check that every node fully contains its child bounds
void sanityCheckAABBRecurse(BVH const& bvh, BVHNode const& node, TrianglePosSet const& triangles) {
    if(node.isLeaf()) {
        // ensure all triangles are inside aabb
        for(unsigned int i = node.first(); i < (node.first() + node.count); i++) {
            TrianglePosition const& t = triangles[bvh.indicies[i]];
            assert(containsTriangle(node.bounds, t));
        }
    }
    else {
        auto const& left = bvh.getNode(node.leftIndex());
        auto const& right = bvh.getNode(node.rightIndex());

        // check both immediate child bounds fit in our bounds
        assert(containsAABB(node.bounds, left.bounds));
        assert(containsAABB(node.bounds, right.bounds));

        sanityCheckAABBRecurse(bvh, left, triangles);
        sanityCheckAABBRecurse(bvh, right, triangles);
    }
}

// walk the whole BVH, explode if the bvh is insane. 
// should compile out on release builds
// this is debug only code, it's certainly not especially efficient
// see also sanityCheck() below for a version that automatically compiles out 
void doSanityCheckBVH(BVH& bvh, TrianglePosSet const& triangles) {
    // check triangle refs are sane
    for(unsigned int i : bvh.indicies) {
        assert(i < triangles.size());
        
        // check uniqueness
        bool found = false;
        for(unsigned int j : bvh.indicies) {
            if(found) {
                assert(j != i);
            } 
            else {
                found = (i == j);
            }
        }
    }

    assert(bvh.nodes.size() >= bvh.nodeCount() + 1);

    // check root node
    if (bvh.root().isLeaf()) {
        assert(bvh.nodeCount() == 1);
    }
    else {
        // first non-root node should be at 2
        assert(bvh.root().leftFirst == 2);

        unsigned int triangleCount = 0;
        // walk nodes
        for(unsigned int i = 2; i < bvh.nodeCount() + 1; i++) {
            auto const& node = bvh.getNode(i);

            if(node.isLeaf()) {
                triangleCount += node.count;
                assert(node.leftFirst < bvh.indicies.size());
                assert(node.leftFirst + node.count <= bvh.indicies.size());
            } 
            else {
                assert(node.leftFirst > i);
                assert(node.leftFirst < bvh.nodeCount() + 2);
            }
        }
        // triangles can be accounted for than once in the bvh. This assert will blow if there's 
        // some missing. but it's no longer actually sufficient. A triangle could be missing all together and
        // another double-counted, and this wouldn't find it. Really need to walk the triangle array, and 
        // search the BVH for every triangle
        assert(triangleCount >= triangles.size());
        std::cout << "bvh triangle count " << triangleCount << " triangles " << triangles.size() << std::endl;
    }

    std::cout << "sanity check recursing " <<std::endl;

    sanityCheckAABBRecurse(bvh, bvh.root(), triangles);

    std::cout << "sanity check OK" <<std::endl;
}

void sanityCheckBVH(BVH& bvh, TrianglePosSet const& triangles) {
#ifndef NDEBUG
    doSanityCheckBVH(bvh, triangles);
#endif
}

