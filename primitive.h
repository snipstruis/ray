#pragma once

#include "utils.h"
#include "basics.h"
#include "material.h"

#include "glm/gtx/io.hpp"
#include "glm/gtx/vector_query.hpp"

#include <vector>
#include <cmath>
#include <cassert>

// Holds the 3 verticies of a triangle.
struct TrianglePos{
    TrianglePos(glm::vec3 const& v1, glm::vec3 const& v2, glm::vec3 const& v3)
        : v{v1, v2, v3} { } 

    float getAverageCoord(unsigned int axis) const {
        assert(axis < 3);
        return (v[0][axis] + v[1][axis] + v[2][axis]) / 3.0f;
    }

    // for a given axis, return the minimum vertex coordinate
    float getMinCoord(unsigned int axis) const {
        assert(axis < 3);
        return std::min(v[0][axis], std::min(v[1][axis], v[2][axis]));
    }

    // for a given axis, return the max vertex coordinate
    float getMaxCoord(unsigned int axis) const {
        assert(axis < 3);
        return std::max(v[0][axis], std::max(v[1][axis], v[2][axis]));
    }

    glm::vec3 getCentroid() const {
        return glm::vec3(getAverageCoord(0), getAverageCoord(1), getAverageCoord(2));
    }

    // not nescessarily the best name - determines if this tri has not become a point or line
    // which seems to happen with some agressive transforms. this trips up the error checking later on
    // note this uses the != operator, so we need to think carefully about this
    bool hasArea() const {
        return (v[0] != v[1]) && (v[1] != v[2]) && (v[2] != v[0]);
    }
     
    float area() const {
        return 0.5f*glm::length(glm::cross(v[1]-v[0], v[2]-v[0]));
    }

    glm::vec3 v[3];
};

inline std::ostream& operator<<(std::ostream& os, const TrianglePos& t) {
    os << "v=(0: " << t.v[0];
    os << " 1: " << t.v[1];
    os << " 2: " << t.v[2];
    os << ")";
    return os;
}

// 3x per-vertex normals, and ref to a material. Separated from TrianglePos to improve cache performance
struct TriangleExtra{
    TriangleExtra(){}
    TriangleExtra(glm::vec3 const& n1, glm::vec3 const& n2, glm::vec3 const& n3, int _mat)
        : n{n1, n2, n3}, mat(_mat) {
            sanityCheck();
        }

    void sanityCheck() const {
        assert(glm::isNormalized(n[0], EPSILON));
        assert(glm::isNormalized(n[1], EPSILON));
        assert(glm::isNormalized(n[2], EPSILON));
    }

    glm::vec3 n[3]; // per-vertex normal
    glm::vec2 t[3]; // tex coords
    int mat; // material
};

// we pass these around a lot, so typedef them out
typedef std::vector<Material> MaterialSet;
typedef std::vector<TriangleExtra> TriangleExtraSet;
typedef std::vector<TrianglePos> TrianglePosSet;

struct Primitives{
    MaterialSet materials;
    // these next two combined define the world triangles. 
    // They are seperate to improve cache performance. Indicies must line up.
    TrianglePosSet pos;
    TriangleExtraSet extra;
    std::vector<int> light_indices;
    std::vector<Texture> textures;
};

// result of an intersection calculation
// This is the former Intersection struct - the basic result of an intersection is now in MiniIntersection
// generally one of these will be built after deciding a specific triangle is the closest
struct FancyIntersection{
    FancyIntersection() = default;
    FancyIntersection(glm::vec3 _impact, 
                      int _mat, 
                      Color color_at_point, 
                      glm::vec3 _normal, 
                      bool _internal)
        : impact(_impact), mat(_mat), diffuse_at_point(color_at_point), normal(_normal), internal(_internal){};

    glm::vec3 impact;   // point of impact
    int mat;            // material at impact
    Color diffuse_at_point;
    glm::vec3 normal;   // normal at impact
    bool internal;
};

inline std::ostream& operator<<(std::ostream& os, const FancyIntersection& i) {
    os << " impact " << i.impact;
    os << " normal " << i.normal;
    os << " internal " << i.internal;

    return os;
}

// adapted from Christer Ericson's Read-Time Collition Detection
inline glm::vec3 barycentric(glm::vec3 p, TrianglePos const& tri) {
    const glm::vec3 v0 = tri.v[1] - tri.v[0];
    const glm::vec3 v1 = tri.v[2] - tri.v[0];
    const glm::vec3 v2 = p - tri.v[0];

    float d00 = glm::dot(v0, v0);
    float d01 = glm::dot(v0, v1);
    float d11 = glm::dot(v1, v1);
    float d20 = glm::dot(v2, v0);
    float d21 = glm::dot(v2, v1);
    float denom = d00 * d11 - d01 * d01;
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;
    return glm::vec3(u,v,w);
}

inline Color textureLookup(Texture const& t, glm::vec2 pixel){
    assert(pixel.x>=0.f);
    assert(pixel.x<=1.f);
    assert(pixel.y>=0.f);
    assert(pixel.y<=1.f);
    assert(t.channels>=3);
    int x = pixel.x*(float)t.w;
    int y = pixel.y*(float)t.h;
    int i = y*t.channels*t.w + x*t.channels;
    return Color(t.pixels[i],
                 t.pixels[i+1],
                 t.pixels[i+2]);
}

// compute triangle/ray intersection
// assumes there is an intersection between t & ray already calculated
inline FancyIntersection FancyIntersect(
        float dist, 
        TrianglePos const& p, 
        TriangleExtra const& t, 
        Ray const& ray,
        bool smooth,
        Primitives const& prim){
    assert(dist < INFINITY);
    t.sanityCheck();

    glm::vec3 hit = ray.origin + ray.direction * dist;
    glm::vec3 bary = barycentric(hit, p);

    // smoothing
    glm::vec3 normal;
    if(smooth){
        normal = glm::normalize(bary.x*t.n[0] + bary.y*t.n[1] + bary.z*t.n[2]);
    }else{
        normal = glm::normalize((t.n[0] + t.n[1] + t.n[2]) / 3.0f);
    }

    // texture lookup
    Material mat = prim.materials[t.mat];
    Color diffColor;
    if(mat.diffuseTexture>=0){
        //printf("(%f,%f) ", t.t[0].x, t.t[0].y);
        //printf("(%f,%f) ", t.t[1].x, t.t[1].y);
        //printf("(%f,%f)\n", t.t[2].x, t.t[2].y);
        //glm::vec2 uv = (bary.x*t.t[0] + bary.y*t.t[1] + bary.z*t.t[2])*(1.f/3.f);
        //if(uv.x!=0.f || uv.y!=0.f) printf("(%f,%f)\n", uv.x, uv.y);
        diffColor = textureLookup(prim.textures[mat.diffuseTexture], glm::vec2(bary.x, bary.y));
        //diffColor = Color(bary.x,bary.y,bary.z);
    }else{
        diffColor = mat.diffuseColor;
    }

    assert(glm::isNormalized(normal, EPSILON));

    // internal check
    bool internal = glm::dot(ray.direction, normal) > 0;
    normal = internal? -normal : normal;

    return FancyIntersection(hit, t.mat, diffColor, normal, internal);
}

// adapted from:
// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
inline float moller_trumbore(TrianglePos const& tri, Ray const& ray) {
    // Find vectors for two edges sharing V1
    glm::vec3 e1 = tri.v[1] - tri.v[0];
    glm::vec3 e2 = tri.v[2] - tri.v[0];

    // Begin calculating determinant - also used to calculate u parameter
    glm::vec3 p = glm::cross(ray.direction, e2);

    // if determinant is near zero, ray lies in plane of triangle or ray is parallel 
    // to plane of triangle
    float det = glm::dot(e1, p);

    // NOT CULLING
    if(det > -EPSILON && det < EPSILON) 
        return INFINITY;

    float inv_det = 1.f / det;

    // calculate distance from V1 to ray origin
    glm::vec3 t = ray.origin - tri.v[0];

    // Calculate u parameter and test bound
    float u = glm::dot(t, p) * inv_det;

    // The intersection lies outside of the triangle
    if(u < 0.f || u > 1.f) 
        return INFINITY;

    // Prepare to test v parameter
    glm::vec3 q = glm::cross(t, e1);

    // Calculate V parameter and test bound
    float v = glm::dot(ray.direction, q) * inv_det;

    // The intersection lies outside of the triangle
    if(v < 0.f || u + v  > 1.f) 
        return INFINITY;

    float ret = glm::dot(e2, q) * inv_det;

    if(ret > EPSILON)  // ray intersection
        return ret;

    // No hit, no win
    return INFINITY;
}

// slice (clip) a triangle by an axis-aligned plane perpendicular to @axis at point @splitPoint
// the two intersection points are returned in @res
// splitPoint must be within the range of the triangle on the given axis
inline void clipTriangle(
    TrianglePos const& t, 
    int axis, 
    float splitPoint, 
    VecPair& res) {
        
    assert(splitPoint > t.getMinCoord(axis));
    assert(splitPoint < t.getMaxCoord(axis));

    // grab the 'other 2' axis..
    int a1 = (axis + 1) % 3;
    int a2 = (axis + 2) % 3;
    unsigned int current = 0;

    for(int i = 0; i < 3; i++) {
        const glm::vec3& v0 = t.v[i];
        const glm::vec3& v1 = t.v[(i + 1) % 3];

        // if at different sides of the splitpoint...
        if(((v0[axis] <= splitPoint) && (v1[axis] > splitPoint)) ||
           ((v1[axis] <= splitPoint) && (v0[axis] > splitPoint))){
            // intersect
            float dx = (v1[axis] - splitPoint) / (v1[axis] - v0[axis]);

            assert(current < res.size());
            res[current][axis] = splitPoint;
            res[current][a1] = ((v1[a1] - v0[a1]) * dx) + v0[a1];
            res[current][a2] = ((v1[a2] - v0[a2]) * dx) + v0[a2];
            current++;
        }
    }
    // should have intersected exactly twice
    assert(current == 2);
}

