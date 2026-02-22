cmake_minimum_required(VERSION 3.10)
project(QtPBRDemo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

# 查找 Qt6 组件：Widgets, OpenGLWidgets, OpenGL
find_package(Qt6 COMPONENTS Widgets OpenGLWidgets OpenGL REQUIRED)
find_package(Assimp REQUIRED)


add_executable(QtPBRDemo main.cpp
        pbrwidget.cpp
        skybox.cpp)

target_link_libraries(QtPBRDemo
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::OpenGL
    assimp::assimp
)

add_custom_command(TARGET QtPBRDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/shader/pbr.vert
        ${CMAKE_CURRENT_SOURCE_DIR}/shader/pbr.frag
        ${CMAKE_CURRENT_SOURCE_DIR}/shader/irradiance.frag
        ${CMAKE_CURRENT_SOURCE_DIR}/shader/conv.vert
        ${CMAKE_CURRENT_SOURCE_DIR}/shader/prefilter.frag
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/pistol.obj
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/Cerberus_A.png
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/Cerberus_N.png
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/Cerberus_RMAC.png
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/skybox.hdr
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/radiance.hdr
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/irrdiance.hdr
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/ibl_brdf_lut.png
        $<TARGET_FILE_DIR:QtPBRDemo>
)