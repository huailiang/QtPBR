#ifndef PBRWIDGET_H
#define PBRWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QVector3D>
#include <QMatrix4x4>
#include <vector>
#include <memory>

// Assimp 前向声明
struct aiNode;
struct aiScene;

class PBRWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_1_Core
{
    Q_OBJECT
public:
    explicit PBRWidget(QWidget *parent = nullptr);
    ~PBRWidget();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    struct Vertex {
        QVector3D position;
        QVector3D normal;
        QVector2D texCoord;
    };

    struct Mesh {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        QOpenGLVertexArrayObject vao;
        QOpenGLBuffer vbo;
        QOpenGLBuffer ebo;
        QVector3D albedo = QVector3D(0.5f, 0.0f, 0.0f);
        float metallic = 0.5f;
        float roughness = 0.5f;

        void setupMesh(QOpenGLFunctions_4_1_Core *gl);
        void draw(QOpenGLShaderProgram &program, QOpenGLFunctions_4_1_Core *gl);
    };

    void loadModel(const QString &path);
    void processAssimpNode(aiNode *node, const aiScene *scene);
    void generateSphereMesh(Mesh &mesh, float radius, int sectors, int stacks);

    QOpenGLShaderProgram m_program;
    std::vector<std::unique_ptr<Mesh>> m_meshes;

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

    // light
    QVector3D m_lightPositions[1];
    QVector3D m_lightColors[1];
};

#endif // PBRWIDGET_H