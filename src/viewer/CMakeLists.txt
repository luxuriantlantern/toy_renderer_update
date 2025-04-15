add_library(TR_LIB_VIEWER
        ${CMAKE_CURRENT_SOURCE_DIR}/include/viewer/viewer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/viewer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/viewer/ui/ui.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/viewer/ui/cameraUI.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/viewer/ui/modelUI.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/viewer/ui/shaderUI.h
)
target_include_directories(TR_LIB_VIEWER PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(TR_LIB_VIEWER PUBLIC
        TR_LIB_CAMERA
        TR_LIB_SCENE
        TR_LIB_RENDER
        third_party # TODO: 不要third_party, 单独拆出来
)