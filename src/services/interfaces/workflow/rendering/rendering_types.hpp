#pragma once

#include <cstdint>

namespace sdl3cpp::services::rendering {

/**
 * @brief GPU-aligned vertex uniform data for textured draw steps.
 * Must match the shader VertexUniforms layout exactly.
 */
struct VertexUniformData {
    float mvp[16];
    float model_mat[16];
    float normal[4];
    float uv_scale[4];
    float camera_pos[4];
    float shadow_vp[16];
};

/**
 * @brief GPU-aligned fragment uniform data for PBR lighting.
 * Must match the shader FragmentUniforms layout exactly (64 bytes, 4x vec4).
 */
struct FragmentUniformData {
    float light_dir[4];
    float light_color[4];
    float ambient[4];
    float material[4];     // x=roughness, y=metallic, zw=unused
    float flash_pos[4];    // xyz=position, w=inner cone cos
    float flash_dir[4];    // xyz=direction, w=outer cone cos
    float flash_color[4];  // rgb=color*intensity, a=range
};

}  // namespace sdl3cpp::services::rendering
