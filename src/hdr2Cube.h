//
// Created by huailiang on 2026/2/22.
//

#ifndef QTPBRDEMO_HDR2CUBE_H
#define QTPBRDEMO_HDR2CUBE_H

#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include "stb_image.h"

// 将 HDR 文件加载为 OpenGL 立方体贴图
inline std::unique_ptr<QOpenGLTexture> loadHDRToCubemap(QOpenGLFunctions *gl, const QString &hdrFile, const int cubemapSize = 1024)
{
    // 1. 加载 HDR 图片
    int width, height, channels;
    float *data = stbi_loadf(hdrFile.toLocal8Bit().data(), &width, &height, &channels, 3); // 强制 3 通道
    if (!data) {
        qWarning() << "Failed to load HDR file:" << hdrFile;
        return nullptr;
    }

    // 2. 创建 2D HDR 纹理
    QOpenGLTexture hdrTexture(QOpenGLTexture::Target2D);
    hdrTexture.setSize(width, height);
    hdrTexture.setFormat(QOpenGLTexture::RGB16F);      // 使用 16 位浮点格式
    hdrTexture.setMinificationFilter(QOpenGLTexture::Linear);
    hdrTexture.setMagnificationFilter(QOpenGLTexture::Linear);
    hdrTexture.setWrapMode(QOpenGLTexture::ClampToEdge);
    hdrTexture.allocateStorage();
    hdrTexture.setData(0, 0, QOpenGLTexture::RGB, QOpenGLTexture::Float32, data);
    stbi_image_free(data);

    // 3. 创建目标立方体贴图
    auto cubemap = std::make_unique<QOpenGLTexture>(QOpenGLTexture::TargetCubeMap);
    cubemap->setSize(cubemapSize, cubemapSize);
    cubemap->setFormat(QOpenGLTexture::RGB16F);
    cubemap->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    cubemap->setMagnificationFilter(QOpenGLTexture::Linear);
    cubemap->setWrapMode(QOpenGLTexture::ClampToEdge);
    cubemap->allocateStorage();

    // 4. 编译转换着色器（顶点和片段）
    QOpenGLShaderProgram convProgram;
    const char *convVS = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        out vec3 WorldPos;
        uniform mat4 projection;
        uniform mat4 view;
        void main() {
            WorldPos = aPos;
            gl_Position = projection * view * vec4(aPos, 1.0);
        }
    )";
    const char *convFS = R"(
        #version 330 core
        out vec4 FragColor;
        in vec3 WorldPos;
        uniform sampler2D hdrEquirectangular;
        const vec2 invAtan = vec2(0.1591, 0.3183);
        vec2 SampleSphericalMap(vec3 v) {
            vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
            uv *= invAtan;
            uv += 0.5;
            return uv;
        }
        void main() {
            vec2 uv = SampleSphericalMap(normalize(WorldPos));
            vec3 color = texture(hdrEquirectangular, uv).rgb;
            FragColor = vec4(color, 1.0);
        }
    )";
    if (!convProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, convVS) ||
        !convProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, convFS) ||
        !convProgram.link()) {
        qWarning() << "Conversion shader error:" << convProgram.log();
        return nullptr;
    }

    // 5. 创建单位立方体 VAO（用于渲染到每个面）
    float cubeVerts[] = {
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
    QOpenGLVertexArrayObject cubeVAO;
    QOpenGLBuffer cubeVBO;
    cubeVAO.create();
    cubeVAO.bind();
    cubeVBO.create();
    cubeVBO.bind();
    cubeVBO.allocate(cubeVerts, sizeof(cubeVerts));
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    cubeVBO.release();
    cubeVAO.release();

    // 6. 设置 FBO 和渲染状态
    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setInternalTextureFormat(GL_RGB16F);
    fboFormat.setAttachment(QOpenGLFramebufferObject::Depth);
    QOpenGLFramebufferObject fbo(cubemapSize, cubemapSize, fboFormat);

    QMatrix4x4 captureProjection;
    captureProjection.perspective(90.0, 1.0, 0.1, 10.0);

    // 六个面的方向与 up 向量
    struct Face { QVector3D target, up; };
    Face faces[6] = {
        { QVector3D( 1.0f,  0.0f,  0.0f), QVector3D(0.0f, -1.0f,  0.0f) }, // +X
        { QVector3D(-1.0f,  0.0f,  0.0f), QVector3D(0.0f, -1.0f,  0.0f) }, // -X
        { QVector3D( 0.0f,  1.0f,  0.0f), QVector3D(0.0f,  0.0f,  1.0f) }, // +Y
        { QVector3D( 0.0f, -1.0f,  0.0f), QVector3D(0.0f,  0.0f, -1.0f) }, // -Y
        { QVector3D( 0.0f,  0.0f,  1.0f), QVector3D(0.0f, -1.0f,  0.0f) }, // +Z
        { QVector3D( 0.0f,  0.0f, -1.0f), QVector3D(0.0f, -1.0f,  0.0f) }  // -Z
    };

    gl->glViewport(0, 0, cubemapSize, cubemapSize);
    gl->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    convProgram.bind();
    convProgram.setUniformValue("hdrEquirectangular", 0);
    hdrTexture.bind(0);

    // 7. 依次渲染六个面
    for (int i = 0; i < 6; ++i) {
        fbo.bind();
        gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                    cubemap->textureId(), 0);
        if (!fbo.isValid()) {
            qWarning() << "FBO incomplete for face" << i;
            fbo.release();
            break;
        }
        gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        QMatrix4x4 captureView;
        captureView.lookAt(QVector3D(0,0,0), faces[i].target, faces[i].up);
        convProgram.setUniformValue("projection", captureProjection);
        convProgram.setUniformValue("view", captureView);

        cubeVAO.bind();
        gl->glDrawArrays(GL_TRIANGLES, 0, 36);
        cubeVAO.release();
        fbo.release();
    }

    QOpenGLFramebufferObject::bindDefault();
    convProgram.release();
    hdrTexture.release();

    // 8. 生成 mipmap 并返回
    cubemap->generateMipMaps();
    return cubemap;
}


#endif //QTPBRDEMO_HDR2CUBE_H