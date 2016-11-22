#pragma once

struct Camera{
    int width, height;
    glm::vec3 eye, screen_center, screen_corner;

    Ray makeRay(int u, int v)
    {
        return Ray(); // silence warning for now
    };
};
