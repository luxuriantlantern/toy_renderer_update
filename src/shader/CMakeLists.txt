add_library(TR_LIB_SHADER
        ${CMAKE_CURRENT_SOURCE_DIR}/include/shader/shader.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/shader/shaderOpenGL.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/shaderOpenGL.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/shader/shaderVulkan.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/shaderVulkan.cpp
)
target_include_directories(TR_LIB_SHADER PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(TR_LIB_SHADER PUBLIC
        EASY_VULKAN
        third_party # TODO: 不要third_party, 单独拆出来
)