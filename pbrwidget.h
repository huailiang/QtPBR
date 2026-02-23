#ifndef PBRWIDGET_H
#define PBRWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QWheelEvent>
#include <vector>
#include <memory>
#include "skybox.h"

struct aiNode;
struct aiScene;

class PBRWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit PBRWidget(QWidget *parent = nullptr);
    ~PBRWidget() override;

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void moveWith(const float delta);

private:
    struct Vertex {
        QVector3D position;
        QVector3D normal;
        QVector2D texCoord;
        QVector3D tangent;
        QVector3D bitangent;
    };

    struct Mesh {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        QOpenGLVertexArrayObject vao;
        QOpenGLBuffer vbo;
        QOpenGLBuffer ebo;

        // 材质属性
        QVector3D albedo = QVector3D(0.5f, 0.0f, 0.0f);
        float metallic = 0.5f;
        float roughness = 0.5f;

        // 纹理
        std::unique_ptr<QOpenGLTexture> albedoTexture;
        std::unique_ptr<QOpenGLTexture> normalTexture;
        std::unique_ptr<QOpenGLTexture> rmacTexture;

        void setupMesh(QOpenGLFunctions *gl);
        void draw(QOpenGLShaderProgram &program, QOpenGLFunctions *gl);
    };

    void loadModel(const QString &path);
    void processAssimpNode(const aiNode *node, const aiScene *scene);
    QOpenGLTexture* loadTexture(const QString &path) const;
    void loadIBLTextures();
    std::unique_ptr<QOpenGLTexture> loadCubemapTexture(const QString &path) const;
    QOpenGLTexture* load2DTexture(const QString &path) const;

    bool generateIrradianceMap(QOpenGLTexture *envCubemap);
    bool generatePrefilterMap(QOpenGLTexture *envCubemap);
    void loadBRDFLUT();

    QOpenGLShaderProgram m_program;          // 主渲染
    QOpenGLShaderProgram m_convProgram;      // 通用转换
    QOpenGLShaderProgram m_irradianceProgram;
    QOpenGLShaderProgram m_prefilterProgram;

    std::vector<std::unique_ptr<Mesh>> m_meshes;
    QOpenGLFunctions *m_glFunc = nullptr;
    QString m_modelDir;

    // camera
    QVector3D m_cameraPos;
    QVector3D m_cameraTarget;
    QVector3D m_cameraUp;
    float m_cameraDistance;
    float m_cameraYaw;
    float m_cameraPitch;
    QPoint m_lastMousePos;
    bool m_mousePressed = false;
    bool m_mouseRightPressed = false;

    QMatrix4x4 m_projection;
    QMatrix4x4 m_view;

    QVector3D m_lightPositions[1];
    QVector3D m_lightColors[1];

    // 天空盒
    std::unique_ptr<SkyBox> m_skybox;

    // IBL 纹理
    std::unique_ptr<QOpenGLTexture> m_irradianceMap;
    std::unique_ptr<QOpenGLTexture> m_prefilterMap;
    QOpenGLTexture* m_brdfLUTTexture;

    // 立方体 VAO（用于渲染到立方体贴图）
    QOpenGLVertexArrayObject m_cubeVAO;
    QOpenGLBuffer m_cubeVBO;
};

#endif // PBRWIDGET_H