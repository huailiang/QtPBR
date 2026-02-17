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
#include <QMouseEvent>
#include <QFile>
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
    m_meshes.clear();
    m_program.release();
    doneCurrent();
}

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

void PBRWidget::initializeGL()
{
    // 获取 OpenGL 函数指针（必须在此调用，因为上下文已激活）
     m_glFunc = QOpenGLContext::currentContext()->functions();
    if (!m_glFunc) {
        qCritical() << "Failed to get OpenGL functions";
        return;
    }

    qDebug() << "OpenGL version:" << QString::fromLatin1((const char*)glGetString(GL_VERSION));

    m_glFunc->glEnable(GL_DEPTH_TEST);

    // 加载着色器源码
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

    // 尝试加载外部模型（默认可执行文件同目录下的 cyborg.obj）
    QString modelPath = "cyborg.obj";
    loadModel(modelPath);

    if (m_meshes.empty()) {
        qDebug() << "No mesh loaded, generating a sphere.";
        auto mesh = std::make_unique<Mesh>();
        generateSphereMesh(*mesh, 1.0f, 64, 64);
        mesh->setupMesh(m_glFunc);  // 传递 OpenGL 函数指针
        m_meshes.push_back(std::move(mesh));
    }

    m_lightPositions[0] = QVector3D(2.0f, 2.0f, 2.0f);
    m_lightColors[0] = QVector3D(300.0f, 300.0f, 300.0f);
}

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

    // 使用 setUniformValueArray 传递 uniform 数组（Qt6 兼容）
    m_program.setUniformValueArray("lightPositions", m_lightPositions, 1);
    m_program.setUniformValueArray("lightColors", m_lightColors, 1);

    for (auto &mesh : m_meshes) {
        mesh->draw(m_program, m_glFunc);
    }

    m_program.release();
}

void PBRWidget::resizeGL(int w, int h)
{
    float aspect = w / (float)h;
    m_projection.setToIdentity();
    m_projection.perspective(45.0f, aspect, 0.1f, 100.0f);
}

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

void PBRWidget::loadModel(const QString &path)
{
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

void PBRWidget::processAssimpNode(aiNode *node, const aiScene *scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
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

            ourMesh->vertices.push_back(vertex);
        }

        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            aiFace face = mesh->mFaces[f];
            for (unsigned int ind = 0; ind < face.mNumIndices; ++ind)
                ourMesh->indices.push_back(face.mIndices[ind]);
        }

        if (mesh->mMaterialIndex >= 0) {
            aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
            aiColor3D color(1.0f, 1.0f, 1.0f);
            material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
            ourMesh->albedo = QVector3D(color.r, color.g, color.b);

            // 简化：金属度和粗糙度使用固定值
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

void PBRWidget::generateSphereMesh(Mesh &mesh, float radius, int sectors, int stacks)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float x, y, z, xy;
    float lengthInv = 1.0f / radius;
    float sectorStep = 2 * M_PI / sectors;
    float stackStep = M_PI / stacks;

    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = M_PI / 2 - i * stackStep;
        xy = radius * cosf(stackAngle);
        z = radius * sinf(stackAngle);

        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = j * sectorStep;
            x = xy * cosf(sectorAngle);
            y = xy * sinf(sectorAngle);

            Vertex vertex;
            vertex.position = QVector3D(x, y, z);
            vertex.normal = QVector3D(x * lengthInv, y * lengthInv, z * lengthInv);
            vertex.texCoord = QVector2D((float)j / sectors, (float)i / stacks);
            vertices.push_back(vertex);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        int k1 = i * (sectors + 1);
        int k2 = k1 + sectors + 1;
        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            if (i != (stacks - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    mesh.vertices = vertices;
    mesh.indices = indices;
    mesh.albedo = QVector3D(0.8f, 0.2f, 0.2f);
    mesh.metallic = 0.1f;
    mesh.roughness = 0.3f;
}

// Mesh::setupMesh 实现
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

    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    gl->glEnableVertexAttribArray(1);
    gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    gl->glEnableVertexAttribArray(2);
    gl->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

    vbo.release();
    vao.release();
}

// Mesh::draw 实现
void PBRWidget::Mesh::draw(QOpenGLShaderProgram &program, QOpenGLFunctions *gl)
{
    program.setUniformValue("albedo", albedo);
    program.setUniformValue("metallic", metallic);
    program.setUniformValue("roughness", roughness);

    vao.bind();
    gl->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
    vao.release();
}