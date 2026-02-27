//
// Created by huailiang on 2026/2/27.
//

#ifndef QTPBRDEMO_COMMON_H
#define QTPBRDEMO_COMMON_H
#include <QFile>
#include <QtCore/qstring.h>
#include <QtCore/qtextstream.h>


//标准视图方向与上方向
struct FaceData {
    QVector3D target; // 相机看向的方向
    QVector3D up;     // 上方向
};

const FaceData faces[6] = {
    { QVector3D( 1.0f,  0.0f,  0.0f), QVector3D(0.0f, -1.0f,  0.0f) }, // +X
    { QVector3D(-1.0f,  0.0f,  0.0f), QVector3D(0.0f, -1.0f,  0.0f) }, // -X
    { QVector3D( 0.0f,  1.0f,  0.0f), QVector3D(0.0f,  0.0f,  1.0f) }, // +Y
    { QVector3D( 0.0f, -1.0f,  0.0f), QVector3D(0.0f,  0.0f, -1.0f) }, // -Y
    { QVector3D( 0.0f,  0.0f,  1.0f), QVector3D(0.0f, -1.0f,  0.0f) }, // +Z
    { QVector3D( 0.0f,  0.0f, -1.0f), QVector3D(0.0f, -1.0f,  0.0f) }  // -Z
};

// ---------- 读取着色器源码 ----------
inline QString readShaderSource(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Failed to open shader file:" << filePath;
        return {};
    }
    QTextStream stream(&file);
    return stream.readAll();
}

#endif //QTPBRDEMO_COMMON_H