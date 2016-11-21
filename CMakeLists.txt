# add a minimum cmake version to silence warning. don't think we _actually_ need 3.5 specifically
cmake_minimum_required(VERSION 3.5)

project(ray)

####### Compile flags
# common flags
set(CMAKE_CXX_FLAGS "-march=native -O3 --std=c++1z -Wall -Wextra -fno-rtti -fno-exceptions")

# release only
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG")

# debug only
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")

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
    v3.h
)

# linking
target_link_libraries(ray SDL2 ${OpenGL_LIBRARY})

# recurse into test
add_subdirectory(test)

