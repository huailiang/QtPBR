cmake_minimum_required(VERSION 3.16)
project(Load3DDemo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 查找 Qt6 组件，需要同时包含 Core, Qml, Quick3D
find_package(Qt6 REQUIRED COMPONENTS Core Qml Quick3D)

add_executable(Load3DDemo
    main.cpp
    resources.qrc
)

# 链接所有需要的 Qt 库
target_link_libraries(Load3DDemo
    Qt6::Core
    Qt6::Qml        # 必须链接，提供 QQmlApplicationEngine
    Qt6::Quick3D
)

# 对于 Qt6，建议添加这一行以导入 QML 插件
qt6_import_qml_plugins(Load3DDemo)