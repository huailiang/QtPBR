import QtQuick 2.15
import QtQuick3D 1.15
import QtQuick3D.Helpers 1.15   // 提供 WASD 控制器

Item {
    width: 1280
    height: 720

    View3D {
        id: view3D
        anchors.fill: parent

        // 1. 场景环境设置：背景颜色
        environment: SceneEnvironment {
            clearColor: "#2b2b2b"   // 深灰色背景
            backgroundMode: SceneEnvironment.Color
        }

        // 2. 相机 - 透视相机，让用户能观察场景
        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 200, 400) // 初始位置：稍高稍远
            clipNear: 1.0
            clipFar: 10000
        }

        // 3. 灯光 - 至少需要一盏灯才能看到模型的颜色
        DirectionalLight {
            eulerRotation.x: -30   // 从上往下照射
            eulerRotation.y: -70   // 从侧面照射，产生阴影效果
            color: Qt.rgba(1.0, 1.0, 0.9, 1.0) // 暖色光
            brightness: 1.0
        }

        // 可选：添加一个点光源增加局部亮度
        PointLight {
            position: Qt.vector3d(100, 200, 200)
            brightness: 2.0
            color: "white"
        }

        // 4. 模型加载 - 这是你关注的重点
        Model {
            id: myModel
            // 使用 Qt 资源系统中的模型文件路径
            // 假设你将模型放在 "models/" 目录下，并通过 .qrc 文件添加
            source: "qrc:/models/your_model.mesh"

            // 如果没有预处理模型，也可以直接加载 glTF 文件 (Qt 6.5+ 支持)
            // source: "qrc:/models/your_model.gltf"

            // 调整模型的位置、缩放和旋转
            position: Qt.vector3d(0, 0, 0)
            scale: Qt.vector3d(1, 1, 1)   // 根据模型实际大小调整
            eulerRotation: Qt.vector3d(0, 0, 0)

            // 材质 - 如果模型自带了材质，这一项可以省略
            // 但如果想手动覆盖材质，可以像下面这样设置
            materials: [
                PrincipledMaterial {
                    baseColor: "#cccccc"   // 基础灰色
                    metalness: 0.1
                    roughness: 0.8
                }
            ]

            // 调试：打印模型加载状态
            onStatusChanged: {
                if (status === Model.Ready) {
                    console.log("Model loaded successfully")
                } else if (status === Model.Error) {
                    console.error("Model loading failed:", errorString)
                }
            }
        }

        // 5. 交互控制器 - 让你能用鼠标和键盘在场景中移动
        WasdController {
            controlledObject: camera
            speed: 200
        }
    }

    // 简单的提示文本
    Text {
        anchors {
            left: parent.left
            bottom: parent.bottom
            margins: 20
        }
        color: "white"
        font.pixelSize: 16
        text: "WASD:移动  鼠标拖动:旋转视角"
        style: Text.Outline
        styleColor: "black"
    }
}