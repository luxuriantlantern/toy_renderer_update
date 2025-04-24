add_library(TR_LIB_RENDER
        ${CMAKE_CURRENT_SOURCE_DIR}/include/render/render.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/render/render_OpenGL.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/render/render_Vulkan.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/render_OpenGL.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/render_Vulkan.cpp
)

target_include_directories(TR_LIB_RENDER PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(TR_LIB_RENDER PUBLIC
        TR_LIB_CAMERA
        TR_LIB_SCENE
        TR_LIB_SHADER
        third_party # TODO: 不要third_party, 单独拆出来
)