QT += quick quick3d

SOURCES += main.cpp
RESOURCES += resources.qrc   # 如果你使用资源文件

target.path = $$[QT_INSTALL_EXAMPLES]/quick3d/load3ddemo
INSTALLS += target