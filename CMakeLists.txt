cmake_minimum_required(VERSION 3.10)
project(QtPBRDemo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

# 查找 Qt6 组件：Widgets, OpenGLWidgets, OpenGL
find_package(Qt6 COMPONENTS Widgets OpenGLWidgets OpenGL REQUIRED)
find_package(Assimp REQUIRED)


add_executable(QtPBRDemo main.cpp pbrwidget.cpp)

target_link_libraries(QtPBRDemo
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::OpenGL
    assimp::assimp
)

add_custom_command(TARGET QtPBRDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/pbr.vert
        ${CMAKE_CURRENT_SOURCE_DIR}/pbr.frag
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/cyborg.obj
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/cyborg_diffuse.png
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/cyborg_normal.png
        $<TARGET_FILE_DIR:QtPBRDemo>
)