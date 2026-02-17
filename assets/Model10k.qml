import QtQuick3D
import QtQuick

Node {
    id: rootNode
    property Material materials
    property var instancing
    property bool useExternalMaterial: false
    property string textureSize: "1024x1024"

    PropertyAnimation on eulerRotation.y {
        from: 0
        to: 360
        duration: 60000
        running: true
        loops: Animation.Infinite
    }

    onUseExternalMaterialChanged: {
        smallMachine.materials = useExternalMaterial
                ? [ materials ] : [ smallMachine_material ]
        propeller1.materials = useExternalMaterial
                ? [ materials ] : [ propeller_material ]
        propeller2.materials = useExternalMaterial
                ? [ materials ] : [ propeller_material ]
        propeller3.materials = useExternalMaterial
                ? [ materials ] : [ propeller_material ]
        propeller4.materials = useExternalMaterial
                ? [ materials ] : [ propeller_material ]
    }

    PrincipledMaterial {
        id: smallMachine_material
        baseColorMap: Texture {
            source: "maps/" + textureSize + "/SmallMachine_Albedo.png"
        }
        normalMap: Texture {
            source: "maps/" + textureSize + "/SmallMachine_Normal.png"
        }
        metalnessMap: Texture {
            source: "maps/" + textureSize + "/SmallMachine_Metalness.png"
        }
        roughnessMap: Texture {
            source: "maps/" + textureSize + "/SmallMachine_Roughness.png"
        }
        roughness: 1.0
        metalness: 1.0
    }

    PrincipledMaterial {
        id: propeller_material
        baseColor: "#b87333"
        metalness: 0.8
        roughness: 0.1
        specularAmount: 1.0
    }

    Model {
        id: smallMachine
        source: "meshes/ship10.mesh"
        scale: Qt.vector3d(0.5, 0.5, 0.5)
        eulerRotation.x: 90
        instancing: rootNode.instancing
        instanceRoot: rootNode
        materials: [
            smallMachine_material
        ]
    }

    Node {
        id: rootNode_1
        scale: Qt.vector3d(0.5, 0.5, 0.5)
        property real propellerRotation: 0

        Model {
            id: propeller4
            scale: Qt.vector3d(0.75, 0.75, 0.75)
            x: -6.00498
            y: 6.00003
            z: -5.01082
            eulerRotation.y: -rootNode_1.propellerRotation
            eulerRotation.x: 90
            source: "meshes/propeller.mesh"
            instancing: rootNode.instancing
            instanceRoot: rootNode
            materials: [
                propeller_material
            ]
        }

        Model {
            id: propeller3
            scale: Qt.vector3d(0.75, 0.75, 0.75)
            x: 5.98832
            y: 6.00002
            z: -4.99438
            eulerRotation.y: rootNode_1.propellerRotation
            eulerRotation.x: 90
            source: "meshes/propeller.mesh"
            instancing: rootNode.instancing
            instanceRoot: rootNode
            materials: [
                propeller_material
            ]
        }

        Model {
            id: propeller2
            scale: Qt.vector3d(0.75, 0.75, 0.75)
            x: 6.00662
            y: 6.00002
            z: 5.0125
            eulerRotation.y: -rootNode_1.propellerRotation
            eulerRotation.x: 90
            source: "meshes/propeller.mesh"
            instancing: rootNode.instancing
            instanceRoot: rootNode
            materials: [
                propeller_material
            ]
        }

        Model {
            id: propeller1
            scale: Qt.vector3d(0.75, 0.75, 0.75)
            x: -6.01134
            y: 6.00002
            z: 5.00359
            eulerRotation.y: rootNode_1.propellerRotation
            eulerRotation.x: 90
            source: "meshes/propeller.mesh"
            instancing: rootNode.instancing
            instanceRoot: rootNode
            materials: [
                propeller_material
            ]
        }

        PropertyAnimation on propellerRotation {
            from: 0
            to: 180
            duration: 250
            loops: Animation.Infinite
            running: true
        }
    }
}
