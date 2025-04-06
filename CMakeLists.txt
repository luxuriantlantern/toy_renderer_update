cmake_minimum_required(VERSION 3.15)
project(toy_renderer_update)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/third_party)
add_executable(${PROJECT_NAME}
        src/main.cpp
)
add_subdirectory(third_party)
add_subdirectory(${PROJECT_SOURCE_DIR}/src)

target_link_libraries(${PROJECT_NAME} PRIVATE
        third_party
)