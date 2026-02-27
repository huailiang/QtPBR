# 源文件列表
set(SOURCES
    main.cpp
    pbrwidget.cpp
    skybox.cpp
        common.h
)

# 项目根目录（相对于当前文件位置）
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(SHADER_DIR ${PROJECT_ROOT}/shader)
set(ASSET_DIR ${PROJECT_ROOT}/assets)

# 着色器文件列表
set(SHADER_FILES
    ${SHADER_DIR}/pbr.vert
    ${SHADER_DIR}/pbr.frag
    ${SHADER_DIR}/sky.vert
    ${SHADER_DIR}/sky.frag
    ${SHADER_DIR}/irradiance.frag
    ${SHADER_DIR}/conv.vert
    ${SHADER_DIR}/conv.frag
    ${SHADER_DIR}/prefilter.frag
)

# 资源文件列表
set(ASSET_FILES
    ${ASSET_DIR}/pistol.obj
    ${ASSET_DIR}/Cerberus_A.png
    ${ASSET_DIR}/Cerberus_N.png
    ${ASSET_DIR}/Cerberus_RMAC.png
    ${ASSET_DIR}/skybox.hdr
    ${ASSET_DIR}/radiance.hdr
    ${ASSET_DIR}/irrdiance.hdr
    ${ASSET_DIR}/ibl_brdf_lut.png
)

add_executable(QtPBRDemo ${SOURCES})

# 添加头文件搜索路径（使当前目录下的 .h 文件可以被包含）
target_include_directories(QtPBRDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(QtPBRDemo
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::OpenGL
    assimp::assimp
)

# 构建后自动拷贝所有着色器和资源文件到可执行文件所在目录
add_custom_command(TARGET QtPBRDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${SHADER_FILES}
        ${ASSET_FILES}
        $<TARGET_FILE_DIR:QtPBRDemo>
    COMMENT "Copying shaders and assets to output directory"
)
