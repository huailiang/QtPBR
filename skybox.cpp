#include "stb_image.h"
#include "skybox.h"

SkyBox::SkyBox() {}
SkyBox::~SkyBox() {
}

void SkyBox::reset() {
    m_vao.release();
    m_vbo.release();
    if (m_cubemapTexture) {
        m_cubemapTexture.release();
    }
    m_program.release();
}

bool SkyBox::initialize(QOpenGLFunctions *gl)
{
    // 1. 编译着色器 (非常简单的着色器，只需传递位置)
    const char *vsrc = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        out vec3 TexCoords;
        uniform mat4 projection;
        uniform mat4 view;
        void main()
        {
            TexCoords = aPos;
            vec4 pos = projection * view * vec4(aPos, 1.0);
            gl_Position = pos.xyww;
        })";
    const char *fsrc = R"(
        #version 330 core
        out vec4 FragColor;
        in vec3 TexCoords;
        uniform samplerCube skybox;
        void main()
        {
            FragColor = texture(skybox, TexCoords);
        })";

    if (!m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, vsrc) ||
        !m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, fsrc) ||
        !m_program.link()) {
        qDebug() << "SkyBox shader error:" << m_program.log();
        return false;
    }

    // 2. 设置天空盒的几何体 (一个单位立方体的顶点)
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

    m_vao.create();
    m_vao.bind();

    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(vertices, sizeof(vertices));

    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    m_vbo.release();
    m_vao.release();

    m_cubemapTexture = std::make_unique<QOpenGLTexture>(QOpenGLTexture::TargetCubeMap);
    m_cubemapTexture->setSize(2048, 2048); // 假设纹理是 1024x1024
    m_cubemapTexture->setFormat(QOpenGLTexture::RGB8_UNorm);
    m_cubemapTexture->allocateStorage();

    // 为六个面分别加载图片, sourceFormat是QOpenGLTexture::BGRA, 这里调整了下效果
    QImage imagePosY("top.jpg");
    m_cubemapTexture->setData(0, 0, QOpenGLTexture::CubeMapPositiveY, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, imagePosY.bits());

    QImage imageNegY("bottom.jpg");
    m_cubemapTexture->setData(0, 0, QOpenGLTexture::CubeMapNegativeY, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, imageNegY.bits());

    QImage imagePosX("right.jpg");
    m_cubemapTexture->setData(0, 0, QOpenGLTexture::CubeMapPositiveX, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, imagePosX.bits());

    QImage imageNegX("left.jpg");
    m_cubemapTexture->setData(0, 0, QOpenGLTexture::CubeMapNegativeX, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, imageNegX.bits());

    QImage imagePosZ("front.jpg");
    m_cubemapTexture->setData(0, 0, QOpenGLTexture::CubeMapPositiveZ, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, imagePosZ.bits());

    QImage imageNegZ("back.jpg");
    m_cubemapTexture->setData(0, 0, QOpenGLTexture::CubeMapNegativeZ, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, imageNegZ.bits());

    m_cubemapTexture->setMinificationFilter(QOpenGLTexture::Linear);
    m_cubemapTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_cubemapTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
    return true;
}

void SkyBox::render(QOpenGLFunctions *gl, const QMatrix4x4 &projection, const QMatrix4x4 &view)
{
    gl->glDepthFunc(GL_LEQUAL); // 天空盒的深度值最大，确保它在背景
    m_program.bind();

    // 移除视图矩阵的平移部分，使天空盒始终跟随相机
    QMatrix4x4 viewNoTranslate = view;
    viewNoTranslate.setColumn(3, QVector4D(0, 0, 0, 1));

    m_program.setUniformValue("projection", projection);
    m_program.setUniformValue("view", viewNoTranslate);

    m_cubemapTexture->bind(0);
    m_program.setUniformValue("skybox", 0);

    m_vao.bind();
    gl->glDrawArrays(GL_TRIANGLES, 0, 36);
    m_vao.release();

    m_program.release();
    gl->glDepthFunc(GL_LESS); // 恢复深度测试函数
}