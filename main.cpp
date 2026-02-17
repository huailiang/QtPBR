#include <QApplication>
#include <QMainWindow>
#include "pbrwidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QMainWindow window;
    window.setWindowTitle("Qt PBR Mesh Demo");
    PBRWidget *pbrWidget = new PBRWidget(&window);
    window.setCentralWidget(pbrWidget);
    window.resize(1024, 768);
    window.show();
    return app.exec();
}