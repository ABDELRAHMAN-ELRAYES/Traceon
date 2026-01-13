cmake_minimum_required(VERSION 4.2.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Traceon VERSION 1.0)

file(GLOB_RECURSE SRC_FILES src/*.cpp)
add_executable(Traceon ${SRC_FILES})