#include <QApplication>
#include <QMainWindow>
#include <QSurfaceFormat>
#include "pbrwidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 设置 OpenGL 格式
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setSamples(4); // 可选抗锯齿
    QSurfaceFormat::setDefaultFormat(format);

    QMainWindow window;
    window.setWindowTitle("Qt6 PBR Mesh Demo");
    PBRWidget *pbrWidget = new PBRWidget(&window);
    window.setCentralWidget(pbrWidget);
    window.resize(1024, 768);
    window.show();
    return app.exec();
}