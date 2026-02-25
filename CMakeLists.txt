cmake_minimum_required(VERSION 3.10)
project(QtPBRDemo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

# 查找 Qt6 组件：Widgets, OpenGLWidgets, OpenGLc
find_package(Qt6 COMPONENTS Widgets OpenGLWidgets OpenGL REQUIRED)
find_package(Assimp REQUIRED)

add_subdirectory(src)