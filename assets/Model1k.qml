import QtQuick3D
import QtQuick

Node {
    id: rootNode
    property Material materials
    property var instancing
    property bool useExternalMaterial: false
    property real gearRotation: 0
    property string textureSize: "" // Not used, just here to prevent error messages

    PropertyAnimation on eulerRotation.y {
        from: 0
        to: 360
        duration: 60000
        running: true
        loops: Animation.Infinite
    }

    onUseExternalMaterialChanged: {
        gear1.materials = useExternalMaterial
                ? [ materials ] : [ parts_material ]
        gear2.materials = useExternalMaterial
                ? [ materials ] : [ parts_material ]
        gear3.materials = useExternalMaterial
                ? [ materials ] : [ parts_material ]
    }

    DefaultMaterial {
        id: parts_material
        diffuseColor: "#ffa3a3a3"
        specularAmount: 0.25
        specularRoughness: 0.25
    }

    Model {
        id: gear3
        y: 0.510341
        z: -1.45991
        eulerRotation.x: -180
        eulerRotation.z: rootNode.gearRotation + 25
        source: "meshes/gear.mesh"
        instancing: rootNode.instancing
        instanceRoot: rootNode
        materials: [
            parts_material
        ]
    }

    Model {
        id: gear2
        x: 0.102771
        z: -0.94508
        eulerRotation.x: 90
        eulerRotation.y: rootNode.gearRotation
        source: "meshes/gear.mesh"
        instancing: rootNode.instancing
        instanceRoot: rootNode
        materials: [
            parts_material
        ]
    }

    Model {
        id: gear1
        eulerRotation.x: 90
        eulerRotation.y: -rootNode.gearRotation
        source: "meshes/gear.mesh"
        instancing: rootNode.instancing
        instanceRoot: rootNode
        materials: [
            parts_material
        ]
    }

    PropertyAnimation on gearRotation {
        from: 0
        to: 360
        duration: 2000
        loops: Animation.Infinite
        running: true
    }
}
