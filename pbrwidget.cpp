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
#include <QWheelEvent>
#include <QFile>
#include <QDebug>
#include <cmath>
#include <vector>
#include <memory>
#include <cstddef>

// 顶点着色器
static const char *vertexShaderSource = R"(
#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 WorldPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    WorldPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;

    gl_Position = projection * view * vec4(WorldPos, 1.0);
}
)";

// 片段着色器（PBR 直接光照）
static const char *fragmentShaderSource = R"(
#version 410 core
out vec4 FragColor;

in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 camPos;

uniform vec3 albedo;
uniform float metallic;
uniform float roughness;

uniform vec3 lightPositions[1];
uniform vec3 lightColors[1];

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - WorldPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 1; ++i)
    {
        vec3 L = normalize(lightPositions[i] - WorldPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;

        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);

        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * albedo;
    vec3 color = ambient + Lo;

    // HDR tonemapping & gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}
)";

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

void PBRWidget::initializeGL()
{
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);

    m_program.create();
    if (!m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource))
        qDebug() << "Vertex shader error:" << m_program.log();
    if (!m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource))
        qDebug() << "Fragment shader error:" << m_program.log();
    if (!m_program.link())
        qDebug() << "Shader link error:" << m_program.log();

    // 尝试加载外部模型（默认可执行文件同目录下的 model.obj）
    QString modelPath = "model.obj";
    loadModel(modelPath);

    if (m_meshes.empty()) {
        qDebug() << "No mesh loaded, generating a sphere.";
        auto mesh = std::make_unique<Mesh>();
        generateSphereMesh(*mesh, 1.0f, 64, 64);
        mesh->setupMesh(this);
        m_meshes.push_back(std::move(mesh));
    }

    m_lightPositions[0] = QVector3D(2.0f, 2.0f, 2.0f);
    m_lightColors[0] = QVector3D(300.0f, 300.0f, 300.0f);
}

void PBRWidget::paintGL()
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
        mesh->draw(m_program, this);
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

    // 直接传递 const aiScene*，不再需要 const_cast
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

            // 简化：金属度和粗糙度使用固定值，实际可从材质自定义属性读取
            ourMesh->metallic = 0.2f;
            ourMesh->roughness = 0.3f;
        }

        ourMesh->setupMesh(this);
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

void PBRWidget::Mesh::setupMesh(QOpenGLFunctions_4_1_Core *gl) {
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

void PBRWidget::Mesh::draw(QOpenGLShaderProgram &program, QOpenGLFunctions_4_1_Core *gl)
{
    program.setUniformValue("albedo", albedo);
    program.setUniformValue("metallic", metallic);
    program.setUniformValue("roughness", roughness);

    vao.bind();
    gl->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
    vao.release();
}