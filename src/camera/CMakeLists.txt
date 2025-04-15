add_library(TR_LIB_CAMERA
    ${CMAKE_CURRENT_SOURCE_DIR}/include/camera/camera.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/camera/orthographic.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/camera/perspective.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/camera.cpp
)

target_include_directories(TR_LIB_CAMERA PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(TR_LIB_CAMERA PUBLIC
    third_party # TODO: 不要third_party, 单独拆出来
)
