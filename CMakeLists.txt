cmake_minimum_required(VERSION 3.16)
project(MyQtApp)

set(CMAKE_CXX_STANDARD 17)

# 告诉 CMake Qt 库的位置（如果创建项目时已填，可省略）
set(CMAKE_PREFIX_PATH "/usr/local/Cellar/qt/6.9.0/lib/cmake")

# 开启 Qt 的自动工具
set(CMAKE_AUTOMOC ON)   # 处理信号与槽
set(CMAKE_AUTOUIC ON)   # 处理 .ui 文件
set(CMAKE_AUTORCC ON)   # 处理 .qrc 资源文件

# 查找并添加必要的 Qt 组件
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# 添加可执行文件
add_executable(MyQtApp
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

# 链接 Qt 库
target_link_libraries(MyQtApp Qt6::Core Qt6::Widgets)