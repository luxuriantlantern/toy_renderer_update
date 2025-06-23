add_subdirectory(camera)
add_subdirectory(render)
add_subdirectory(scene)
add_subdirectory(shader)
add_subdirectory(viewer)
add_subdirectory(EasyVulkan)


add_executable(TR_EXE_TEST ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp)
add_executable(TR_EXE_MAIN ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
add_executable(TR_EXE_TEST2 ${CMAKE_CURRENT_SOURCE_DIR}/test2.cpp)
add_executable(TR_EXE_TEST3 ${CMAKE_CURRENT_SOURCE_DIR}/test3.cpp)

target_link_libraries(TR_EXE_MAIN PUBLIC
    TR_LIB_CAMERA
    TR_LIB_RENDER
    TR_LIB_SCENE
    TR_LIB_SHADER
    TR_LIB_VIEWER
    EASY_VULKAN
    third_party
)

target_link_libraries(TR_EXE_TEST PUBLIC
    TR_LIB_CAMERA
    TR_LIB_RENDER
    TR_LIB_SCENE
    TR_LIB_SHADER
    TR_LIB_VIEWER
    EASY_VULKAN
    third_party
)

target_link_libraries(TR_EXE_TEST2 PUBLIC
    TR_LIB_CAMERA
    TR_LIB_RENDER
    TR_LIB_SCENE
    TR_LIB_SHADER
    TR_LIB_VIEWER
    EASY_VULKAN
    third_party
)

target_link_libraries(TR_EXE_TEST3 PUBLIC
        TR_LIB_CAMERA
        TR_LIB_RENDER
        TR_LIB_SCENE
        TR_LIB_SHADER
        TR_LIB_VIEWER
        EASY_VULKAN
        third_party
)