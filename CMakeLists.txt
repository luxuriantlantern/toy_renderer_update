cmake_minimum_required(VERSION 3.28)
project(toy_renderer_update)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(third_party)
add_subdirectory(${PROJECT_SOURCE_DIR}/src)