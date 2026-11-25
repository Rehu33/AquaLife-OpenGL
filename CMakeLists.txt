cmake_minimum_required(VERSION 3.10)
project(AquaLife CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(aqualife
    src/main.cpp
    src/common.cpp
    src/fish.cpp
    src/environment.cpp
    src/water.cpp
    src/particles.cpp
)

target_include_directories(aqualife PRIVATE include)

if(WIN32)
    target_link_libraries(aqualife PRIVATE freeglut opengl32 glu32)
else()
    target_link_libraries(aqualife PRIVATE glut GL GLU)
endif()