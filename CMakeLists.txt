# add a minimum cmake version to silence warning. don't think we _actually_ need 3.5 specifically
cmake_minimum_required(VERSION 3.5)

project(ray)

find_package(OpenGL)

####### Compile flags
# common flags

message("SDL include dir ${SDL_INCLUDE_DIR}")
message("SDL lib ${SDL_LIBRARY}")
message("SDL found ${SDL_FOUND}")

if(NOT WIN32)
    set(CMAKE_CXX_FLAGS "-fopenmp -march=native -O3 --std=c++1y -Wall -Wextra -fno-rtti -fno-exceptions -Wno-unused-parameter")

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
    include_directories("template/lib/SDL2-2.0.3/include")
    link_directories("template/lib/SDL2-2.0.3/lib/x86")
endif()


# quieten warnings in glm - we're using radians
add_definitions("-DGLM_FORCE_RADIANS")

MESSAGE(STATUS "Using OpenGL lib ${OpenGL_LIBRARY}")

# add the root directory of the project as an include dir
include_directories(.)

# source files
add_executable(ray 
    main.cc
    tiny_obj_loader.cc
    scene.h
)

# linking
target_link_libraries(ray SDL2 ${OPENGL_gl_LIBRARY})

# create a target to auto load the raytracer 
# note that it will start this with the wd set to source, so it can find any test objects
add_custom_target(run-ray ray DEPENDS ray WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} USES_TERMINAL)

# recurse into test
if(NOT WIN32)
    add_subdirectory(test)
endif()

