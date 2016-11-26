# add a minimum cmake version to silence warning. don't think we _actually_ need 3.5 specifically
cmake_minimum_required(VERSION 3.5)

project(ray)

####### Compile flags
# common flags
set(CMAKE_CXX_FLAGS "-fopenmp -march=native -O3 --std=c++1y -Wall -Wextra -fno-rtti -fno-exceptions -Wno-unused-parameter")

# explicitly turn on color diags as this doesn't work automatically under ninja 
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")

# turn this on to enable bounds checking in the stl, but it hurts perf a lot, it seems
#add_definitions("-D_GLIBCXX_DEBUG")

# release only
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG")

# debug only
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")

# quieten warnings in glm - we're using radians
add_definitions("-DGLM_FORCE_RADIANS")

###### Platform specifics
if(UNIX)
    if(APPLE)
    	FIND_LIBRARY(OpenGL_LIBRARY OpenGL)
    else()
	# assume this means linux for now (sorry *bsd)
    	FIND_LIBRARY(OpenGL_LIBRARY GL)
    endif()
elseif(WIN32)
    MESSAGE(FATAL_ERROR "windows not handled yet")
else()
    MESSAGE(FATAL_ERROR "unknown OS")
endif()

MESSAGE(STATUS "Using OpenGL lib ${OpenGL_LIBRARY}")

# add the root directory of the project as an include dir
include_directories(.)

# source files
add_executable(ray 
    main.cc
    scene.h
)

# linking
target_link_libraries(ray SDL2 ${OpenGL_LIBRARY})

# recurse into test
add_subdirectory(test)

