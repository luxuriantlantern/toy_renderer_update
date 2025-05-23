add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/glm)

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(glfw)

add_library(glad STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c
        ${CMAKE_CURRENT_SOURCE_DIR}/glad/include/glad/glad.h
        ${CMAKE_CURRENT_SOURCE_DIR}/glad/include/KHR/khrplatform.h
)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glad/include)

find_package(Vulkan REQUIRED)

add_library(imgui
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/backends/imgui_impl_glfw.cpp
        imgui/backends/imgui_impl_opengl3.cpp
        imgui/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
        ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include
        ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(imgui PUBLIC Vulkan::Vulkan)

add_library(ImGuiFileDialog ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiFileDialog/ImGuiFileDialog.cpp)
target_include_directories(ImGuiFileDialog PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiFileDialog
)

add_library(nlohmann_json INTERFACE)

target_include_directories(nlohmann_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/json/include)

add_subdirectory(SPIRV-Headers)
add_subdirectory(SPIRV-Tools)
add_subdirectory(glslang)

add_library(third_party_headers INTERFACE)
target_include_directories(third_party_headers INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_library(third_party INTERFACE)
target_link_libraries(third_party INTERFACE
        glad
        glfw
        imgui
        glm
        ImGuiFileDialog
        nlohmann_json
        glslang
        glslang-default-resource-limits
        third_party_headers
)