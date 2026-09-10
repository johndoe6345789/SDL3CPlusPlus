#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Must match SSAOParams in postfx_ssao.frag.metal
struct SSAOUniformData {
    float projection[16];
    float inv_projection[16];
    float params[4];       // radius, bias, 1/width, 1/height
    float kernel[16 * 4];  // 16 float4 samples
};

/**
 * @brief Returns the frame's SSAO output texture, (re)creating it in
 *        `context` under "postfx_ssao_texture" whenever it is missing or
 *        no longer matches `width`/`height`.
 */
SDL_GPUTexture* GetOrCreateSsaoTexture(WorkflowContext& context,
                                       SDL_GPUDevice* device,
                                       uint32_t width, uint32_t height);

/**
 * @brief Builds the SSAO fragment uniform block from the current
 *        projection matrix, frame size and sample kernel.
 *
 * @return false if `kernel` holds fewer than the 64 floats (16 float4
 *         samples) the shader expects, leaving `out` untouched.
 */
bool BuildSsaoUniforms(const glm::mat4& proj, uint32_t width,
                       uint32_t height, const std::vector<float>& kernel,
                       SSAOUniformData& out);

}  // namespace sdl3cpp::services::impl
