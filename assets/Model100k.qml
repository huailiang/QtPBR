import QtQuick3D
import QtQuick

Model {
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
        city100.materials = useExternalMaterial
                ? [ materials,
                   materials,
                   materials,
                   materials,
                   materials,
                   materials,
                   materials,
                   materials,
                   materials,
                   materials ]
                : [ house_material,
                   propeller_material,
                   power_material,
                   electric_material,
                   parts_material,
                   cockpit_material,
                   gyro_material,
                   engine_material,
                   base_material,
                   frame_material ]
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
        id: house_material
        baseColorMap: Texture {
            source: "maps/" + textureSize + "/House_Albedo.png"
        }
        normalMap: Texture {
            source: "maps/" + textureSize + "/House_Normal.png"
        }
        metalnessMap: Texture {
            source: "maps/" + textureSize + "/House_Metalness.png"
        }
        roughnessMap: Texture {
            source: "maps/" + textureSize + "/House_Roughness.png"
        }
        roughness: 1.0
        metalness: 1.0
    }

    PrincipledMaterial {
        id: propeller_material
        baseColorMap: Texture {
            source: "maps/" + textureSize + "/Propeller_Albedo.png"
        }
        normalMap: Texture {
            source: "maps/" + textureSize + "/Propeller_Normal.png"
        }
        metalnessMap: Texture {
            source: "maps/" + textureSize + "/Propeller_Metalness.png"
        }
        roughnessMap: Texture {
            source: "maps/" + textureSize + "/Propeller_Roughness.png"
        }
        roughness: 1.0
        metalness: 1.0
    }

    PrincipledMaterial {
        id: power_material
        baseColorMap: Texture {
            source: "maps/" + textureSize + "/Power_Albedo.png"
        }
        normalMap: Texture {
            source: "maps/" + textureSize + "/Power_Normal.png"
        }
        metalnessMap: Texture {
            source: "maps/" + textureSize + "/Power_Metalness.png"
        }
        roughnessMap: Texture {
            source: "maps/" + textureSize + "/Power_Roughness.png"
        }
        roughness: 1.0
        metalness: 1.0
    }

    PrincipledMaterial {
        id: electric_material
        baseColorMap: Texture {
            source: "maps/" + textureSize + "/Electric_Albedo.png"
        }
        normalMap: Texture {
            source: "maps/" + textureSize + "/Electric_Normal.png"
        }
        metalnessMap: Texture {
            source: "maps/" + textureSize + "/Electric_Metalness.png"
        }
        roughnessMap: Texture {
            source: "maps/" + textureSize + "/Electric_Roughness.png"
        }
        roughness: 1.0
        metalness: 1.0
    }

    PrincipledMaterial {
        id: parts_material
        baseColorMap: Texture {
            source: "maps/" + textureSize + "/Parts_Albedo.png"
        }
        normalMap: Texture {
            source: "maps/" + textureSize + "/Parts_Normal.png"
        }
        metalnessMap: Texture {
            source: "maps/" + textureSize + "/Parts_Metalness.png"
        }
        roughnessMap: Texture {
            source: "maps/" + textureSize + "/Parts_Roughness.png"
        }
        roughness: 1.0
        metalness: 1.0
    }

    PrincipledMaterial {
        id: cockpit_material
        baseColorMap: Texture {
            source: "maps/" + textureSize + "/Cockpit_Albedo.png"
        }
        normalMap: Texture {
            source: "maps/" + textureSize + "/Cockpit_Normal.png"
        }
        metalnessMap: Texture {
            source: "maps/" + textureSize + "/Cockpit_Metalness.png"
        }
        roughnessMap: Texture {
            source: "maps/" + textureSize + "/Cockpit_Roughness.png"
        }
        roughness: 1.0
        metalness: 1.0
    }

    PrincipledMaterial {
        id: gyro_material
        baseColorMap: Texture {
            source: "maps/" + textureSize + "/Gyro_Albedo.png"
        }
        normalMap: Texture {
            source: "maps/" + textureSize + "/Gyro_Normal.png"
        }
        metalnessMap: Texture {
            source: "maps/" + textureSize + "/Gyro_Metalness.png"
        }
        roughnessMap: Texture {
            source: "maps/" + textureSize + "/Gyro_Roughness.png"
        }
        roughness: 1.0
        metalness: 1.0
    }

    PrincipledMaterial {
        id: engine_material
        baseColorMap: Texture {
            source: "maps/" + textureSize + "/Engine_Albedo.png"
        }
        normalMap: Texture {
            source: "maps/" + textureSize + "/Engine_Normal.png"
        }
        metalnessMap: Texture {
            source: "maps/" + textureSize + "/Engine_Metalness.png"
        }
        roughnessMap: Texture {
            source: "maps/" + textureSize + "/Engine_Roughness.png"
        }
        roughness: 1.0
        metalness: 1.0
    }

    PrincipledMaterial {
        id: base_material
        baseColorMap: Texture {
            source: "maps/" + textureSize + "/StreetBase_Albedo.png"
        }
        normalMap: Texture {
            source: "maps/" + textureSize + "/StreetBase_Normal.png"
        }
        metalnessMap: Texture {
            source: "maps/" + textureSize + "/StreetBase_Metalness.png"
        }
        roughnessMap: Texture {
            source: "maps/" + textureSize + "/StreetBase_Roughness.png"
        }
        roughness: 1.0
        metalness: 1.0
    }

    PrincipledMaterial {
        id: frame_material
        baseColorMap: Texture {
            source: "maps/" + textureSize + "/Frame_Albedo.png"
        }
        normalMap: Texture {
            source: "maps/" + textureSize + "/Frame_Normal.png"
        }
        metalnessMap: Texture {
            source: "maps/" + textureSize + "/Frame_Metalness.png"
        }
        roughnessMap: Texture {
            source: "maps/" + textureSize + "/Frame_Roughness.png"
        }
        roughness: 1.0
        metalness: 1.0
    }

    Model {
        id: city100
        eulerRotation.x: 90
        scale: Qt.vector3d(0.2, 0.2, 0.2)
        source: "meshes/city100.mesh"
        instancing: rootNode.instancing
        instanceRoot: rootNode
        materials: [
            house_material,
            propeller_material,
            power_material,
            electric_material,
            parts_material,
            cockpit_material,
            gyro_material,
            engine_material,
            base_material,
            frame_material
        ]
    }

    Node {
        id: rootNode_1
        scale: Qt.vector3d(0.2, 0.2, 0.2)
        property real propellerRotation: 0

        Model {
            id: propeller3
            x: -10.2263
            y: 9.04872
            z: -10.1482
            eulerRotation.x: 90
            eulerRotation.y: rootNode_1.propellerRotation
            source: "meshes/propeller.mesh"
            instancing: rootNode.instancing
            instanceRoot: rootNode
            materials: [
                propeller_material
            ]
        }

        Model {
            id: propeller2
            x: 10.1482
            y: 9.04872
            z: -10.2263
            eulerRotation.x: 90
            eulerRotation.y: -rootNode_1.propellerRotation
            source: "meshes/propeller.mesh"
            instancing: rootNode.instancing
            instanceRoot: rootNode
            materials: [
                propeller_material
            ]
        }

        Model {
            id: propeller1
            x: 10.2263
            y: 9.04872
            z: 10.1482
            eulerRotation.x: 90
            eulerRotation.y: rootNode_1.propellerRotation
            source: "meshes/propeller.mesh"
            instancing: rootNode.instancing
            instanceRoot: rootNode
            materials: [
                propeller_material
            ]
        }

        Model {
            id: propeller4
            x: -10.1482
            y: 9.04872
            z: 10.2263
            eulerRotation.x: 90
            eulerRotation.y: -rootNode_1.propellerRotation
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
