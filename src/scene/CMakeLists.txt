add_library(TR_LIB_SCENE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/scene/scene.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/scene/object.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/scene.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/object.cpp
)
target_include_directories(TR_LIB_SCENE PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(TR_LIB_SCENE PUBLIC
        TR_LIB_CAMERA
        third_party # TODO: 不要third_party, 单独拆出来
)