#ifndef SKYBOX_H
#define SKYBOX_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QString>
#include <memory>

class SkyBox
{
public:
    SkyBox() = default;
    ~SkyBox() = default;

    // 初始化：从 HDR 文件生成立方体贴图
    bool initialize(QOpenGLFunctions *gl);

    // 渲染天空盒
    void render(QOpenGLFunctions *gl, const QMatrix4x4 &projection, const QMatrix4x4 &view);

    void reset();

    QOpenGLTexture* getCubemapTexture() const
    {
        return m_cubemapTexture.get();
    }

private:
    // 着色器程序
    QOpenGLShaderProgram m_convProgram;    // 用于 HDR -> 立方体贴图的转换
    QOpenGLShaderProgram m_skyboxProgram;  // 用于最终渲染

    // 几何体
    QOpenGLVertexArrayObject m_cubeVAO;
    QOpenGLBuffer m_cubeVBO;

    // 纹理
    std::unique_ptr<QOpenGLTexture> m_cubemapTexture;

    // 辅助函数
    bool createCubeGeometry(QOpenGLFunctions *gl);
    bool compileShaderPrograms(QOpenGLFunctions *gl);
    bool generateCubemapFromHDR(QOpenGLFunctions *gl, const QString &hdrFile);
};

#endif // SKYBOX_H