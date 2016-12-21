# add a minimum cmake version to silence warning. don't think we _actually_ need 3.5 specifically
cmake_minimum_required(VERSION 3.5)

project(ray)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type specified - setting to debug")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build Type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
else()
    message(STATUS "build type ${CMAKE_BUILD_TYPE}")
endif()

find_package(OpenGL)
#find_package(OpenCL)

MESSAGE(STATUS "Using OpenGL include dir ${OPENGL_INCLUDE_DIR}")
MESSAGE(STATUS "Using OpenGL lib ${OPENGL_gl_LIBRARY}")
#MESSAGE(STATUS "Using OpenCL include dir ${OpenCL_INCLUDE_DIRS}")
#MESSAGE(STATUS "Using OpenCL libs ${OpenCL_LIBRARIES}")

####### Compile flags
# common flags

if(NOT WIN32)
    set(CMAKE_CXX_FLAGS "-march=native --std=c++1z -Wall -Wextra -fno-rtti -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")

    # explicitly turn on color diags as this doesn't work automatically under ninja 
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")

    # release only
    set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")

    # debug only
    set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")

ELSE()
# win32 flags

ENDIF()

# turn this on to enable bounds checking in the stl, but it hurts perf a lot, it seems
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "STL debugging is on")
    add_definitions("-D_GLIBCXX_DEBUG")
else()
    message(STATUS "STL debugging is off")
endif()

if(WIN32)
    include_directories("lib/winOnly/SDL2-2.0.3")
    link_directories("lib/winOnly/SDL2-2.0.3/lib/x86")
endif()

# make the library dir a root include path
include_directories("lib")

# quieten warnings in glm - we're using radians
add_definitions("-DGLM_FORCE_RADIANS")

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

#add_subdirectory(report)

set(IMG_OUTPUT_DIR "$ENV{HOME}")
message(STATUS "image output dir ${IMG_OUTPUT_DIR}")

# create a target to auto load the raytracer 
# note that it will start this with the wd set to source, so it can find any test objects

macro(add_scene key scenefile)
    set(SUFFIX "ray-${key}") 

    # add run-ray-XYZ target
    add_custom_target("run-${SUFFIX}"
        COMMAND ray ${scenefile} ${IMG_OUTPUT_DIR}
        DEPENDS ray 
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/data
        USES_TERMINAL
        )

    # add batch-ray-XYZ target (doesn't do anything yet)
    add_custom_target("batch-${SUFFIX}"
        COMMAND ray -b ${scenefile} ${IMG_OUTPUT_DIR}
        DEPENDS ray 
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/data
        USES_TERMINAL
        )

    # add debug-ray-XYZ target
    # assumes debugger is lldb
    # FIXME: assumes binary name
    add_custom_target("debug-${SUFFIX}"
        COMMAND lldb ${CMAKE_CURRENT_BINARY_DIR}/ray -- ${scenefile} ${IMG_OUTPUT_DIR}
        DEPENDS ray 
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/data
        USES_TERMINAL
        )

endmacro(add_scene)    

# so, this first line creates a new target run-ray-point for the file scene/point.scene
add_scene("point" "point.scene")
add_scene("bunny" "bunny.scene")
add_scene("bunny-45" "bunny-45.scene")
add_scene("dragon" "dragon.scene")
add_scene("dragon2" "dragon2.scene")
add_scene("lone-sphere" "lone-sphere.scene")
add_scene("spots" "spots.scene")
add_scene("spots2" "spots2.scene")
add_scene("spots3" "spots3.scene")
add_scene("teapot" "teapot.scene")
add_scene("4teapots" "4teapots.scene")
add_scene("4teapots-plane" "4teapots-plane.scene")
add_scene("plane" "plane.scene")
add_scene("teapot-plane" "teapot-plane.scene")
add_scene("teapot-no-transform" "teapot-no-transform.scene")
add_scene("teapot-hi" "teapot-hi.scene")
add_scene("col-lights" "col-lights.scene")
add_scene("tree" "tree.scene")
add_scene("cubes" "cubes.scene")
add_scene("cube" "cube.scene")
add_scene("buddha" "buddha.scene")
add_scene("sportscar" "sportscar.scene")
add_scene("forest" "forest.scene")
add_scene("teddy" "teddy.scene")
add_scene("pumpkin" "pumpkin.scene")
add_scene("falling-trees" "falling-trees.scene")

