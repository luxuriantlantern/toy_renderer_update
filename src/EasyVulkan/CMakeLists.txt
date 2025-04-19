add_library(EASY_VULKAN
        ${CMAKE_CURRENT_SOURCE_DIR}/include/EasyVulkan/EasyVKStart.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/EasyVulkan/easyVulkan.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/EasyVulkan/GlfwGeneral.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/EasyVulkan/VKBase.h
)
target_include_directories(EASY_VULKAN PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(Vulkan REQUIRED)

target_link_libraries(EASY_VULKAN PUBLIC
        third_party
        Vulkan::Vulkan
)