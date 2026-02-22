#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "pbrwidget.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QMouseEvent>
#include <QFile>
#include <QFileInfo>
#include <vector>
#include <memory>
#include <cstddef>
#include "hdr2Cube.h"

PBRWidget::PBRWidget(QWidget *parent)
    : QOpenGLWidget(parent), m_brdfLUTTexture(nullptr) {
    m_cameraDistance = 100.0f;
    m_cameraYaw = 0.0f;
    m_cameraPitch = 0.0f;
    m_cameraPos = QVector3D(0, 0, 5);
    m_cameraTarget = QVector3D(0, 0, 0);
    m_cameraUp = QVector3D(0, 1, 0);
}

PBRWidget::~PBRWidget()
{
    makeCurrent();
    m_skybox.reset();
    m_irradianceMap.reset();
    m_prefilterMap.reset();
    delete m_brdfLUTTexture;
    m_brdfLUTTexture = nullptr;
    m_meshes.clear(); // 自动释放纹理和 OpenGL 资源
    m_program.release();
    m_convProgram.release();
    m_irradianceProgram.release();
    m_prefilterProgram.release();
    doneCurrent();
}

// ---------- 读取着色器源码 ----------
static QString readShaderSource(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Failed to open shader file:" << filePath;
        return QString();
    }
    QTextStream stream(&file);
    return stream.readAll();
}

QOpenGLTexture* PBRWidget::loadTexture(const QString &path) const {
    QImage image(path);
    image = image.convertToFormat(QImage::Format_RGBA8888);
    auto texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    texture->setData(image);
    texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);
    texture->setWrapMode(QOpenGLTexture::Repeat);
    return texture;
}

void PBRWidget::loadIBLTextures()
{
    // 加载辐照度贴图
    m_irradianceMap = loadCubemapTexture("irradiance.hdr");
    if (m_irradianceMap) {
        m_irradianceMap->setMinificationFilter(QOpenGLTexture::Linear);
        m_irradianceMap->setMagnificationFilter(QOpenGLTexture::Linear);
        m_irradianceMap->setWrapMode(QOpenGLTexture::ClampToEdge);
    }

    // 加载预滤波贴图（需启用 mipmap 过滤）
    m_prefilterMap = loadCubemapTexture("prefilter.hdr");
    if (m_prefilterMap) {
        m_prefilterMap->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_prefilterMap->setMagnificationFilter(QOpenGLTexture::Linear);
        m_prefilterMap->setWrapMode(QOpenGLTexture::ClampToEdge);
    }

    // 加载 BRDF LUT
    m_brdfLUTTexture = load2DTexture("ibl_brdf_lut.png");
    if (m_brdfLUTTexture) {
        m_brdfLUTTexture->setMinificationFilter(QOpenGLTexture::Linear);
        m_brdfLUTTexture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_brdfLUTTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
    }
}

std::unique_ptr<QOpenGLTexture> PBRWidget::loadCubemapTexture(const QString &path) const
{
    auto texture = loadHDRToCubemap(m_glFunc, path);
    if (texture == nullptr) {
        qWarning() << "Failed to load cubemap:" << path;
    }
    return texture;
}

QOpenGLTexture* PBRWidget::load2DTexture(const QString &path) const
{
    const QImage image(path);
    if (image.isNull()) {
        qWarning() << "Failed to load 2D texture:" << path;
        return nullptr;
    }
    auto texture = std::make_unique<QOpenGLTexture>(image.convertToFormat(QImage::Format_RGBA8888));
    return texture.release();
}

bool PBRWidget::generateIrradianceMap(QOpenGLTexture *envCubemap)
{
    constexpr int size = 32;
    m_irradianceMap = std::make_unique<QOpenGLTexture>(QOpenGLTexture::TargetCubeMap);
    m_irradianceMap->setSize(size, size);
    m_irradianceMap->setFormat(QOpenGLTexture::RGB16F);
    m_irradianceMap->allocateStorage();

    if (!m_irradianceProgram.isLinked()) {
        m_irradianceProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, readShaderSource("conv.vert"));
        m_irradianceProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, readShaderSource("irradiance.frag"));
        if (!m_irradianceProgram.link()) {
            qDebug() << "Irradiance shader error:" << m_irradianceProgram.log();
            return false;
        }
    }

    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setInternalTextureFormat(GL_RGB16F);
    fboFormat.setAttachment(QOpenGLFramebufferObject::Depth);
    QOpenGLFramebufferObject fbo(size, size, fboFormat);
    QMatrix4x4 captureProjection;
    captureProjection.perspective(90.0, 1.0, 0.1, 10.0);

    struct Face { QVector3D target, up; };
    Face faces[6] = {
        { QVector3D( 1,0,0), QVector3D(0,-1,0) },
        { QVector3D(-1,0,0), QVector3D(0,-1,0) },
        { QVector3D( 0,1,0), QVector3D(0,0,1) },
        { QVector3D( 0,-1,0), QVector3D(0,0,-1) },
        { QVector3D( 0,0,1), QVector3D(0,-1,0) },
        { QVector3D( 0,0,-1), QVector3D(0,-1,0) }
    };

    m_glFunc->glViewport(0, 0, size, size);
    m_irradianceProgram.bind();
    m_irradianceProgram.setUniformValue("environmentMap", 0);
    envCubemap->bind(0);

    for (int i = 0; i < 6; ++i) {
        fbo.bind();
        m_glFunc->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                          GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                          m_irradianceMap->textureId(), 0);
        if (!fbo.isValid()) { fbo.release(); break; }
        m_glFunc->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        QMatrix4x4 captureView;
        captureView.lookAt(QVector3D(0,0,0), faces[i].target, faces[i].up);
        m_irradianceProgram.setUniformValue("projection", captureProjection);
        m_irradianceProgram.setUniformValue("view", captureView);

        m_cubeVAO.bind();
        m_glFunc->glDrawArrays(GL_TRIANGLES, 0, 36);
        m_cubeVAO.release();

        fbo.release();
    }

    QOpenGLFramebufferObject::bindDefault();
    m_irradianceProgram.release();
    envCubemap->release();

     m_irradianceMap->setMinificationFilter(QOpenGLTexture::Linear);
    m_irradianceMap->setMagnificationFilter(QOpenGLTexture::Linear);
    m_irradianceMap->setWrapMode(QOpenGLTexture::ClampToEdge);
    return true;
}

bool PBRWidget::generatePrefilterMap(QOpenGLTexture *envCubemap)
{
    constexpr int baseSize = 128;
    constexpr int maxMipLevels = 5;
    m_prefilterMap = std::make_unique<QOpenGLTexture>(QOpenGLTexture::TargetCubeMap);
    m_prefilterMap->setSize(baseSize, baseSize);
    m_prefilterMap->setFormat(QOpenGLTexture::RGB16F);
    m_prefilterMap->allocateStorage();
    m_prefilterMap->generateMipMaps();

    if (!m_prefilterProgram.isLinked()) {
        m_prefilterProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, readShaderSource("conv.vert"));
        m_prefilterProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, readShaderSource("prefilter.frag"));
        if (!m_prefilterProgram.link()) {
            qDebug() << "Prefilter shader error:" << m_prefilterProgram.log();
            return false;
        }
    }

    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setInternalTextureFormat(GL_RGB16F);
    fboFormat.setAttachment(QOpenGLFramebufferObject::Depth);

    QMatrix4x4 captureProjection;
    captureProjection.perspective(90.0, 1.0, 0.1, 10.0);
    struct Face { QVector3D target, up; };
    Face faces[6] = {
        { QVector3D( 1,0,0), QVector3D(0,-1,0) },
        { QVector3D(-1,0,0), QVector3D(0,-1,0) },
        { QVector3D( 0,1,0), QVector3D(0,0,1) },
        { QVector3D( 0,-1,0), QVector3D(0,0,-1) },
        { QVector3D( 0,0,1), QVector3D(0,-1,0) },
        { QVector3D( 0,0,-1), QVector3D(0,-1,0) }
    };
    m_prefilterProgram.bind();
    m_prefilterProgram.setUniformValue("environmentMap", 0);
    envCubemap->bind(0);

    for (int mip = 0; mip < maxMipLevels; ++mip) {
        int mipSize = baseSize / pow(2, mip);
        float roughness = (float)mip / (float)(maxMipLevels - 1);

        QOpenGLFramebufferObject fbo(mipSize, mipSize, fboFormat);
        m_glFunc->glViewport(0, 0, mipSize, mipSize);

        for (int i = 0; i < 6; ++i) {
            fbo.bind();
            m_glFunc->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                              GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                              m_prefilterMap->textureId(), mip);
            if (!fbo.isValid()) continue;
            m_glFunc->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            QMatrix4x4 captureView;
            captureView.lookAt(QVector3D(0,0,0), faces[i].target, faces[i].up);
            m_prefilterProgram.setUniformValue("projection", captureProjection);
            m_prefilterProgram.setUniformValue("view", captureView);
            m_prefilterProgram.setUniformValue("roughness", roughness);

            m_cubeVAO.bind();
            m_glFunc->glDrawArrays(GL_TRIANGLES, 0, 36);
            m_cubeVAO.release();

            fbo.release();
        }
    }
    QOpenGLFramebufferObject::bindDefault();
    m_prefilterProgram.release();
    envCubemap->release();

    m_prefilterMap->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    m_prefilterMap->setMagnificationFilter(QOpenGLTexture::Linear);
    m_prefilterMap->setWrapMode(QOpenGLTexture::ClampToEdge);
    return true;
}

void PBRWidget::initializeGL()
{
    m_glFunc = QOpenGLContext::currentContext()->functions();
    if (!m_glFunc) {
        qCritical() << "Failed to get OpenGL functions";
        return;
    }
    qDebug() << "OpenGL version:" << QString::fromLatin1((const char*)glGetString(GL_VERSION));
    m_glFunc->glEnable(GL_DEPTH_TEST);
    const QString vertexSource = readShaderSource("pbr.vert");
    const QString fragmentSource = readShaderSource("pbr.frag");
    if (vertexSource.isEmpty() || fragmentSource.isEmpty()) {
        qCritical() << "Main shader source loading failed.";
        return;
    }

    m_program.create();
    if (!m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexSource))
        qDebug() << "Vertex shader error:" << m_program.log();
    if (!m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSource))
        qDebug() << "Fragment shader error:" << m_program.log();
    if (!m_program.link())
        qDebug() << "Shader link error:" << m_program.log();
    loadModel("pistol.obj");
    if (m_meshes.empty()) {
        qDebug() << "No mesh loaded.";
    }
    m_lightPositions[0] = QVector3D(2.0f, 2.0f, 2.0f);
    m_lightColors[0] = QVector3D(300.0f, 300.0f, 300.0f);


    constexpr float cubeVerts[] = {
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
         1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f }; // 36个顶点数据
    m_cubeVAO.create(); m_cubeVAO.bind();
    m_cubeVBO.create(); m_cubeVBO.bind();
    m_cubeVBO.allocate(cubeVerts, sizeof(cubeVerts));
    m_glFunc->glEnableVertexAttribArray(0);
    m_glFunc->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), 0);
    m_cubeVBO.release(); m_cubeVAO.release();

    m_skybox = std::make_unique<SkyBox>();
    if (!m_skybox->initialize(m_glFunc)) {
        qDebug() << "Failed to initialize skybox.";
    }
    if (m_skybox && m_skybox->getCubemapTexture()) {
        generateIrradianceMap(m_skybox->getCubemapTexture());
        generatePrefilterMap(m_skybox->getCubemapTexture());
    }
    loadBRDFLUT();
}

void PBRWidget::loadBRDFLUT()
{
    QImage img("ibl_brdf_lut.png");
    if (img.isNull()) { qWarning() << "BRDF LUT missing"; return; }
    img = img.convertToFormat(QImage::Format_RGBA8888);
    m_brdfLUTTexture = new QOpenGLTexture(img);
    m_brdfLUTTexture->setMinificationFilter(QOpenGLTexture::Linear);
    m_brdfLUTTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_brdfLUTTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
}

void PBRWidget::paintGL()
{
    m_glFunc->glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    m_glFunc->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_program.bind();
    m_program.setUniformValue("camPos", m_cameraPos);
    m_program.setUniformValue("projection", m_projection);
    m_program.setUniformValue("view", m_view);
    const QMatrix4x4 model;
    m_program.setUniformValue("model", model);
    m_program.setUniformValueArray("lightPositions", m_lightPositions, 1);
    m_program.setUniformValueArray("lightColors", m_lightColors, 1);

    for (const auto &mesh : m_meshes) {
        mesh->draw(m_program, m_glFunc);
    }
    if (m_skybox) {
        m_skybox->render(m_glFunc, m_projection, m_view);
    }

     // 绑定 IBL 纹理
    if (m_irradianceMap) {
        m_irradianceMap->bind(2);
        m_program.setUniformValue("irradianceMap", 2);
    }
    if (m_prefilterMap) {
        m_prefilterMap->bind(3);
        m_program.setUniformValue("prefilterMap", 3);
    }
    if (m_brdfLUTTexture) {
        m_brdfLUTTexture->bind(4);
        m_program.setUniformValue("brdfLUT", 4);
    }
    m_program.release();
}

// ---------- 窗口大小改变 ----------
void PBRWidget::resizeGL(const int w, const int h)
{
    const float aspect = w / static_cast<float>(h);
    m_projection.setToIdentity();
    m_projection.perspective(60.0f, aspect, 0.1f, 240.0f);
}

// ---------- 鼠标事件 ----------
void PBRWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = true;
        m_lastMousePos = event->pos();
    } else if (event->button() == Qt::RightButton) {
        m_mouseRightPressed = true;
        m_lastMousePos = event->pos();
    }
}

void PBRWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPoint delta = event->pos() - m_lastMousePos;
    if (m_mousePressed) {
        float sensitivity = 0.005f;
        m_cameraYaw -= delta.x() * sensitivity;
        m_cameraPitch += delta.y() * sensitivity;
        const float pitchLimit = M_PI / 2 - 0.01f;
        if (m_cameraPitch > pitchLimit)
            m_cameraPitch = pitchLimit;
        if (m_cameraPitch < -pitchLimit)
            m_cameraPitch = -pitchLimit;

        float x = m_cameraDistance * cos(m_cameraYaw) * cos(m_cameraPitch);
        float y = m_cameraDistance * sin(m_cameraPitch);
        float z = m_cameraDistance * sin(m_cameraYaw) * cos(m_cameraPitch);
        m_cameraPos = QVector3D(x, y, z) + m_cameraTarget;

        m_view.setToIdentity();
        m_view.lookAt(m_cameraPos, m_cameraTarget, m_cameraUp);
        update();
    } else if (m_mouseRightPressed) {
        float sensitivity = 0.005f * m_cameraDistance;
        QVector3D right = QVector3D::crossProduct(m_cameraTarget - m_cameraPos, m_cameraUp).normalized();
        QVector3D up = QVector3D::crossProduct(right, (m_cameraTarget - m_cameraPos).normalized()).normalized();
        m_cameraTarget += right * delta.x() * sensitivity;
        m_cameraTarget += up * delta.y() * sensitivity;

        QVector3D dir = (m_cameraPos - m_cameraTarget).normalized();
        m_cameraPos = m_cameraTarget + dir * m_cameraDistance;

        m_view.setToIdentity();
        m_view.lookAt(m_cameraPos, m_cameraTarget, m_cameraUp);
        update();
    }
    m_lastMousePos = event->pos();
}

void PBRWidget::wheelEvent(QWheelEvent *event)
{
    const float delta = event->angleDelta().y() / 120.0f;
    m_cameraDistance -= delta * 1.5f;
    if (m_cameraDistance < 50.0f)
        m_cameraDistance = 50.0f;
    if (m_cameraDistance > 200.0f)
        m_cameraDistance = 200.0f;

    const float x = m_cameraDistance * cos(m_cameraYaw) * cos(m_cameraPitch);
    const float y = m_cameraDistance * sin(m_cameraPitch);
    const float z = m_cameraDistance * sin(m_cameraYaw) * cos(m_cameraPitch);
    m_cameraPos = QVector3D(x, y, z) + m_cameraTarget;
    m_view.setToIdentity();
    m_view.lookAt(m_cameraPos, m_cameraTarget, m_cameraUp);
    update();
}

void PBRWidget::loadModel(const QString &path)
{
    const QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        qDebug() << "Model file does not exist:" << path;
        return;
    }
    m_modelDir = fileInfo.absolutePath();
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path.toStdString(),
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace
    );
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        qDebug() << "Assimp error:" << importer.GetErrorString();
        return;
    }
    processAssimpNode(scene->mRootNode, scene);
}

void PBRWidget::processAssimpNode(const aiNode *node, const aiScene *scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        const aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        auto ourMesh = std::make_unique<Mesh>();
        for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
            Vertex vertex;
            vertex.position = QVector3D(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);
            if (mesh->HasNormals())
                vertex.normal = QVector3D(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
            if (mesh->HasTextureCoords(0))
                vertex.texCoord = QVector2D(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
            else
                vertex.texCoord = QVector2D(0.0f, 0.0f);

            // 提取切线/双切线（需要 Assimp 的 aiProcess_CalcTangentSpace 标志）
            if (mesh->HasTangentsAndBitangents()) {
                vertex.tangent = QVector3D(mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z);
                vertex.bitangent = QVector3D(mesh->mBitangents[v].x, mesh->mBitangents[v].y, mesh->mBitangents[v].z);
            } else {
                vertex.tangent = QVector3D(1,0,0);
                vertex.bitangent = QVector3D(0,1,0);
            }
            ourMesh->vertices.push_back(vertex);
        }

        // 索引数据
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace face = mesh->mFaces[f];
            for (unsigned int ind = 0; ind < face.mNumIndices; ++ind)
                ourMesh->indices.push_back(face.mIndices[ind]);
        }

        // 材质处理
        if (mesh->mMaterialIndex >= 0) {
            const aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
            // 获取反照率颜色（后备）
            aiColor3D color(1.0f, 1.0f, 1.0f);
            material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
            ourMesh->albedo = QVector3D(color.r, color.g, color.b);

            // 尝试加载反照率纹理
            QOpenGLTexture *tex = loadTexture("Cerberus_A.png");
            ourMesh->albedoTexture.reset(tex);
            delete tex;

            tex = loadTexture("Cerberus_N.png");
            ourMesh->normalTexture.reset(tex);
            delete tex;

            tex = loadTexture("Cerberus_RMAC.png");
            ourMesh->rmacTexture.reset(tex);
            delete tex;

            // 金属度和粗糙度仍使用固定值（可扩展为纹理）
            ourMesh->metallic = 0.2f;
            ourMesh->roughness = 0.3f;
        }
        ourMesh->setupMesh(m_glFunc);
        m_meshes.push_back(std::move(ourMesh));
    }
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processAssimpNode(node->mChildren[i], scene);
    }
}

void PBRWidget::Mesh::setupMesh(QOpenGLFunctions *gl)
{
    vao.create();
    vao.bind();

    vbo = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo.create();
    vbo.bind();
    vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo.allocate(vertices.data(), vertices.size() * sizeof(Vertex));

    ebo = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    ebo.create();
    ebo.bind();
    ebo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ebo.allocate(indices.data(), indices.size() * sizeof(unsigned int));

    // position
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    // normal
    gl->glEnableVertexAttribArray(1);
    gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    // texCoord
    gl->glEnableVertexAttribArray(2);
    gl->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    // tangent
    gl->glEnableVertexAttribArray(3);
    gl->glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
    // bitangent
    gl->glEnableVertexAttribArray(4);
    gl->glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));

    vbo.release();
    vao.release();
}

void PBRWidget::Mesh::draw(QOpenGLShaderProgram &program, QOpenGLFunctions *gl)
{
    program.setUniformValue("metallic", metallic);
    program.setUniformValue("roughness", roughness);

    if (albedoTexture && albedoTexture->isCreated() && albedoTexture->textureId() != 0) {
        albedoTexture->bind(0);
        program.setUniformValue("albedoMap", 0);
        program.setUniformValue("useAlbedoMap", true);
    } else {
        program.setUniformValue("albedo", albedo);
        program.setUniformValue("useAlbedoMap", false);
    }

    if (normalTexture) {
        normalTexture->bind(1);
        program.setUniformValue("normalMap", 1);
        program.setUniformValue("useNormalMap", true);
    } else {
        program.setUniformValue("useNormalMap", false);
    }

    if (rmacTexture) {
        rmacTexture->bind(2);
        program.setUniformValue("rmacMap", 1);
        program.setUniformValue("useRmacMap", true);
    } else {
        program.setUniformValue("useRmacMap", false);
    }

    vao.bind();
    gl->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
    vao.release();

    if (albedoTexture) albedoTexture->release();
    if (normalTexture) normalTexture->release();
}