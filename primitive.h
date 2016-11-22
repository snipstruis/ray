#pragma once

struct Primitive{
    v3 pos;
    Material mat;
    virtual float intersects(Ray) = 0;
};

struct Plane:Primitive{
    v3 normal;
    virtual float intersect(Ray r){
        return 9999999999;
    };
};

struct Sphere:Primitive{
    float radius;
    virtual float intersect(Ray r){
        return 9999999999;
    };
};

