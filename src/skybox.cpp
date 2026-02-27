#include "skybox.h"
#include <QImage>
#include <QOpenGLFramebufferObject>
#include <QDebug>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "common.h"

bool SkyBox::initialize(QOpenGLFunctions *gl)
{
    createCubeGeometry(gl);

    if (!compileShaderPrograms(gl))
        return false;

    if (!generateCubemapFromHDR(gl, "skybox.hdr"))
        return false;

    return true;
}

void SkyBox::reset() {
    if (m_cubemapTexture) {
        m_cubemapTexture.release();
    }
    m_convProgram.release();
    m_skyboxProgram.release();
}

bool SkyBox::createCubeGeometry(QOpenGLFunctions *gl)
{
    // 单位立方体的 36 个顶点（6 个面 * 2 个三角形 * 3 个顶点）
    constexpr float vertices[] = {
        -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
    };

    m_cubeVAO.create();
    m_cubeVAO.bind();

    m_cubeVBO.create();
    m_cubeVBO.bind();
    m_cubeVBO.allocate(vertices, sizeof(vertices));

    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    m_cubeVBO.release();
    m_cubeVAO.release();
    return true;
}

bool SkyBox::compileShaderPrograms(QOpenGLFunctions *gl)
{
    if (!m_convProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, readShaderSource("conv.vert")) ||
        !m_convProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, readShaderSource("conv.frag")) ||
        !m_convProgram.link()) {
        qDebug() << "Conv shader error:" << m_convProgram.log();
        return false;
    }

    if (!m_skyboxProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, readShaderSource("sky.vert")) ||
        !m_skyboxProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, readShaderSource("sky.frag")) ||
        !m_skyboxProgram.link()) {
        qDebug() << "Skybox shader error:" << m_skyboxProgram.log();
        return false;
    }

    return true;
}
bool SkyBox::generateCubemapFromHDR(QOpenGLFunctions *gl, const QString &hdrFile)
{
    // 1. 加载 HDR 图片
    int width, height, channels;
    float *data = stbi_loadf(hdrFile.toLocal8Bit().data(), &width, &height, &channels, 3);
    if (!data) {
        qWarning() << "Failed to load HDR file:" << hdrFile;
        return false;
    }

    // 2. 创建 2D 纹理存放 HDR 经纬图
    QOpenGLTexture hdrTexture(QOpenGLTexture::Target2D);
    hdrTexture.setSize(width, height);
    hdrTexture.setFormat(QOpenGLTexture::RGB16F);
    hdrTexture.setMinificationFilter(QOpenGLTexture::Linear);
    hdrTexture.setMagnificationFilter(QOpenGLTexture::Linear);
    hdrTexture.setWrapMode(QOpenGLTexture::ClampToEdge);
    hdrTexture.allocateStorage();
    hdrTexture.setData(0, 0, QOpenGLTexture::RGB, QOpenGLTexture::Float32, data);
    stbi_image_free(data);

    // 3. 创建目标立方体贴图
    constexpr int cubemapSize = 1024;
    m_cubemapTexture = std::make_unique<QOpenGLTexture>(QOpenGLTexture::TargetCubeMap);
    m_cubemapTexture->setSize(cubemapSize, cubemapSize);
    m_cubemapTexture->setFormat(QOpenGLTexture::RGB16F);
    m_cubemapTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    m_cubemapTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_cubemapTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
    m_cubemapTexture->allocateStorage();

    // 4. 设置 FBO（包含深度缓冲）
    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setInternalTextureFormat(GL_RGB16F);
    fboFormat.setAttachment(QOpenGLFramebufferObject::Depth);
    QOpenGLFramebufferObject fbo(cubemapSize, cubemapSize, fboFormat);

    // 5. 投影矩阵：90° 视场，宽高比 1:1，近远平面适当
    QMatrix4x4 captureProjection;
    captureProjection.perspective(90.0, 1.0, 0.1, 10.0);

    // 6. 绑定转换着色器
    m_convProgram.bind();
    m_convProgram.setUniformValue("hdrEquirectangular", 0);
    hdrTexture.bind(0);

    gl->glViewport(0, 0, cubemapSize, cubemapSize);
    gl->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // 7. 逐个面渲染
    for (int i = 0; i < 6; ++i) {
        fbo.bind();
        // 将当前立方体贴图面附加到 FBO 颜色附着
        gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                   m_cubemapTexture->textureId(), 0);
        if (!fbo.isValid()) {
            qWarning() << "FBO incomplete for face" << i;
            fbo.release();
            break;
        }
        gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // 构建视图矩阵：相机位于原点，看向 target，上方向为 up
        QMatrix4x4 captureView;
        captureView.lookAt(QVector3D(0, 0, 0), faces[i].target, faces[i].up);

        m_convProgram.setUniformValue("projection", captureProjection);
        m_convProgram.setUniformValue("view", captureView);

        // 绘制单位立方体
        m_cubeVAO.bind();
        gl->glDrawArrays(GL_TRIANGLES, 0, 36);
        m_cubeVAO.release();

        fbo.release();
    }

    // 8. 恢复默认帧缓冲并生成 mipmap
    QOpenGLFramebufferObject::bindDefault();
    m_cubemapTexture->generateMipMaps();

    m_convProgram.release();
    hdrTexture.release();
    return true;
}

void SkyBox::render(QOpenGLFunctions *gl, const QMatrix4x4 &projection, const QMatrix4x4 &view)
{
    if (!m_cubemapTexture)
        return;

    gl->glDepthFunc(GL_LEQUAL);

    m_skyboxProgram.bind();

    QMatrix4x4 viewNoTranslate = view;
    viewNoTranslate.setColumn(3, QVector4D(0, 0, 0, 1));
    m_skyboxProgram.setUniformValue("projection", projection);
    m_skyboxProgram.setUniformValue("view", viewNoTranslate);

    m_cubemapTexture->bind(0);
    m_skyboxProgram.setUniformValue("skybox", 0);

    m_cubeVAO.bind();
    gl->glDrawArrays(GL_TRIANGLES, 0, 36);
    m_cubeVAO.release();

    m_skyboxProgram.release();
    gl->glDepthFunc(GL_LESS);
}