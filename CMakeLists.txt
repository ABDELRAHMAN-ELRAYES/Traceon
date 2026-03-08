cmake_minimum_required(VERSION 4.2.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Traceon VERSION 1.0)

# Add src and subfolders to include path
include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/backend
    ${PROJECT_SOURCE_DIR}/src/backend
    ${PROJECT_SOURCE_DIR}/src/backend/core
    ${PROJECT_SOURCE_DIR}/src/backend/utils
    ${PROJECT_SOURCE_DIR}/src/backend/decoder
)

# Add all cpp files
file(GLOB_RECURSE SRC_FILES src/*.cpp)

add_executable(Traceon ${SRC_FILES})