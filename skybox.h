#ifndef SKYBOX_H
#define SKYBOX_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QString>
#include <memory>
#include <QVector3D>
#include <QMatrix4x4>

class SkyBox
{
public:
    SkyBox();
    ~SkyBox();


    void reset();

    // 初始化：加载立方体贴图纹理和编译着色器
    bool initialize(QOpenGLFunctions *gl);
    // 渲染天空盒
    void render(QOpenGLFunctions *gl, const QMatrix4x4 &projection, const QMatrix4x4 &view);

private:
    QOpenGLShaderProgram m_program;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vbo;
    std::unique_ptr<QOpenGLTexture> m_cubemapTexture;
};

#endif // SKYBOX_H