
# add a minimum cmake version to silence warning. don't think we _actually_ need 3.7 specifically
cmake_minimum_required(VERSION 3.7)

project(ray)

# see http://blog.alexbeutel.com/285/cmake-opengl-and-glx-on-os-x/
FIND_LIBRARY(OpenGL_LIBRARY OpenGL)

add_executable(ray main.cc)

target_link_libraries(ray SDL2 ${OpenGL_LIBRARY})


