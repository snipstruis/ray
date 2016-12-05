# add a minimum cmake version to silence warning. don't think we _actually_ need 3.5 specifically
cmake_minimum_required(VERSION 3.5)

project(ray)

message(STATUS "build type ${CMAKE_BUILD_TYPE}")

find_package(OpenGL)

####### Compile flags
# common flags

if(NOT WIN32)
    set(CMAKE_CXX_FLAGS "-fopenmp -march=native -O3 --std=c++1y -Wall -Wextra -fno-rtti -Wno-unused-parameter")

    # explicitly turn on color diags as this doesn't work automatically under ninja 
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")

    # release only
    set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG")

    # debug only
    set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")

ELSE()
# win32 flags

ENDIF()

# turn this on to enable bounds checking in the stl, but it hurts perf a lot, it seems
#add_definitions("-D_GLIBCXX_DEBUG")

#add_definitions("-DTEA_TIME_FOR_MRS_NESBIT")

if(WIN32)
    include_directories("lib/winOnly/SDL2-2.0.3")
    link_directories("lib/winOnly/SDL2-2.0.3/lib/x86")
endif()

# make the library dir a root include path
include_directories("lib")

# quieten warnings in glm - we're using radians
add_definitions("-DGLM_FORCE_RADIANS")

MESSAGE(STATUS "Using OpenGL lib ${OPENGL_gl_LIBRARY}")

# add the root directory of the project as an include dir
include_directories(.)

# source files
add_executable(ray 
    main.cc
    loader.cc
    tiny_obj_loader.cc
    scene.h
)

# linking
target_link_libraries(ray SDL2 ${OPENGL_gl_LIBRARY})


# recurse into test
if(NOT WIN32)
    add_subdirectory(test)
endif()

# create a target to auto load the raytracer 
# note that it will start this with the wd set to source, so it can find any test objects

macro(add_scene key scenefile)
    set(N "run-ray-${key}") 

    message(STATUS "adding target ${N} for scenefile ${scenefile}")

    add_custom_target(${N}
        COMMAND ray ${scenefile} 
        DEPENDS ray 
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} 
        USES_TERMINAL
        )
endmacro(add_scene)    

# so, this first line creates a new target run-ray-point for the file scene/point.scene
add_scene("point" "scene/point.scene")
add_scene("lone-sphere" "scene/lone-sphere.scene")
add_scene("spot" "scene/spot.scene")

