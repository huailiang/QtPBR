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

PBRWidget::PBRWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    m_cameraDistance = 5.0f;
    m_cameraYaw = 0.0f;
    m_cameraPitch = 0.0f;
    m_cameraPos = QVector3D(0,0,5);
    m_cameraTarget = QVector3D(0,0,0);
    m_cameraUp = QVector3D(0,1,0);
}

PBRWidget::~PBRWidget()
{
    makeCurrent();
    m_meshes.clear(); // 自动释放纹理和 OpenGL 资源
    m_program.release();
    doneCurrent();
}

// ---------- 辅助函数：读取着色器源码 ----------
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

// ---------- 辅助函数：加载纹理 ----------
QOpenGLTexture* PBRWidget::loadTexture(const QString &path) {
    QImage image;
    if (!image.load(path)) {
        qWarning() << "Failed to load texture:" << path;
        return nullptr;
    }
    // 转换为 RGBA 格式并翻转 Y 轴（OpenGL 原点在左下）
    image = image.convertToFormat(QImage::Format_RGBA8888);
    // 翻转图像以适应 OpenGL 坐标系 (原点在左下角)
    // image = image.flipped(Qt::Vertical);

    auto texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    texture->setData(image);
    texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);
    texture->setWrapMode(QOpenGLTexture::Repeat);
    return texture;
}

// ---------- 初始化 OpenGL ----------
void PBRWidget::initializeGL()
{
    m_glFunc = QOpenGLContext::currentContext()->functions();
    if (!m_glFunc) {
        qCritical() << "Failed to get OpenGL functions";
        return;
    }

    qDebug() << "OpenGL version:" << QString::fromLatin1((const char*)glGetString(GL_VERSION));

    m_glFunc->glEnable(GL_DEPTH_TEST);

    // 加载着色器
    QString vertexSource = readShaderSource("pbr.vert");
    QString fragmentSource = readShaderSource("pbr.frag");
    if (vertexSource.isEmpty() || fragmentSource.isEmpty()) {
        qCritical() << "Shader source loading failed.";
        return;
    }

    m_program.create();
    if (!m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexSource))
        qDebug() << "Vertex shader error:" << m_program.log();
    if (!m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSource))
        qDebug() << "Fragment shader error:" << m_program.log();
    if (!m_program.link())
        qDebug() << "Shader link error:" << m_program.log();

    loadModel("cyborg.obj");
    if (m_meshes.empty()) {
        qDebug() << "No mesh loaded.";
    }
    m_lightPositions[0] = QVector3D(2.0f, 2.0f, 2.0f);
    m_lightColors[0] = QVector3D(300.0f, 300.0f, 300.0f);
}

// ---------- 绘制 ----------
void PBRWidget::paintGL()
{
    m_glFunc->glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    m_glFunc->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_program.bind();

    m_program.setUniformValue("camPos", m_cameraPos);
    m_program.setUniformValue("projection", m_projection);
    m_program.setUniformValue("view", m_view);
    QMatrix4x4 model;
    m_program.setUniformValue("model", model);

    m_program.setUniformValueArray("lightPositions", m_lightPositions, 1);
    m_program.setUniformValueArray("lightColors", m_lightColors, 1);

    for (auto &mesh : m_meshes) {
        mesh->draw(m_program, m_glFunc);
    }

    m_program.release();
}

// ---------- 窗口大小改变 ----------
void PBRWidget::resizeGL(const int w, const int h)
{
    const float aspect = w / (float)h;
    m_projection.setToIdentity();
    m_projection.perspective(45.0f, aspect, 0.1f, 100.0f);
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
    float delta = event->angleDelta().y() / 120.0f;
    m_cameraDistance -= delta * 0.5f;
    if (m_cameraDistance < 1.0f)
        m_cameraDistance = 1.0f;
    if (m_cameraDistance > 20.0f)
        m_cameraDistance = 20.0f;

    float x = m_cameraDistance * cos(m_cameraYaw) * cos(m_cameraPitch);
    float y = m_cameraDistance * sin(m_cameraPitch);
    float z = m_cameraDistance * sin(m_cameraYaw) * cos(m_cameraPitch);
    m_cameraPos = QVector3D(x, y, z) + m_cameraTarget;

    m_view.setToIdentity();
    m_view.lookAt(m_cameraPos, m_cameraTarget, m_cameraUp);
    update();
}

// ---------- 加载模型 ----------
void PBRWidget::loadModel(const QString &path)
{
    const QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        qDebug() << "Model file does not exist:" << path;
        return;
    }
    m_modelDir = fileInfo.absolutePath();  // 保存模型目录

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

// ---------- 递归处理 Assimp 节点 ----------
void PBRWidget::processAssimpNode(aiNode *node, const aiScene *scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        auto ourMesh = std::make_unique<Mesh>();

        // 顶点数据
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
            aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

            // 获取反照率颜色（后备）
            aiColor3D color(1.0f, 1.0f, 1.0f);
            material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
            ourMesh->albedo = QVector3D(color.r, color.g, color.b);

            // 尝试加载反照率纹理
            QOpenGLTexture *tex = loadTexture("cyborg_diffuse.png");
            if (tex) {
                ourMesh->albedoTexture.reset(tex);
            } else {
                qDebug() << "Failed to load albedo texture";
            }

            tex = loadTexture("cyborg_normal.png");
            if (tex) {
                ourMesh->normalTexture.reset(tex);
            } else {
                qDebug() << "Failed to load normal texture";
            }
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

// ---------- Mesh::setupMesh 实现 ----------
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

// ---------- Mesh::draw 实现 ----------
void PBRWidget::Mesh::draw(QOpenGLShaderProgram &program, QOpenGLFunctions *gl)
{
    program.setUniformValue("metallic", metallic);
    program.setUniformValue("roughness", roughness);

    if (albedoTexture) {
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

    vao.bind();
    gl->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
    vao.release();

    if (albedoTexture) albedoTexture->release();
    if (normalTexture) normalTexture->release();
}