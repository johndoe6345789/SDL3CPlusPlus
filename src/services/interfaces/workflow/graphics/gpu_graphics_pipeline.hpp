#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <SDL3/SDL_gpu.h>
#include <array>
#include <string>

namespace sdl3cpp::services::impl {

/// graphics.gpu.pipeline.create's parameters, all with their defaults.
struct GpuPipelineCreateParams {
    std::string vertexShaderKey   = "vertex_shader";
    std::string fragmentShaderKey = "fragment_shader";
    std::string vertexFormat      = "position_color";
    std::string pipelineKey       = "gpu_pipeline";
    bool depthWrite               = true;
    bool depthTest                = true;
    std::string cullMode          = "back";
    float depthBias               = 0.0f;
    float depthBiasSlope          = 0.0f;
    int numColorTargets           = 1;
    std::string depthFormat       = "d32_float";
    bool releaseShaders           = true;
    std::string colorFormat       = "swapchain";
    bool hasDepth                 = true;
    bool alphaBlend               = false;
};

/// Reads GpuPipelineCreateParams from the step's parameters, applying the
/// struct's defaults for anything absent or of the wrong type.
GpuPipelineCreateParams ReadGpuPipelineCreateParams(
    const WorkflowStepDefinition& step);

/// Vertex buffer/attribute storage for one vertex layout, owned by the
/// caller so pointers into it stay valid until pipeline creation.
struct GpuVertexAttributeLayout {
    SDL_GPUVertexBufferDescription vbufDesc     = {};
    std::array<SDL_GPUVertexAttribute, 4> attrs = {};
    Uint32 numBuffers                           = 0;
    Uint32 numAttributes                        = 0;
};

/**
 * @brief Builds the vertex buffer/attribute layout for a named format.
 *
 * Supported formats: "none" (fullscreen triangle, no vertex buffers),
 * "position_uv_lmuv_normal" (BSP: pos+uv+lmuv+normal, 40 bytes),
 * "position_uv" (pos+uv, 20 bytes), and the default "position_color"
 * (pos+ubyte4 color, 16 bytes).
 */
GpuVertexAttributeLayout BuildVertexAttributeLayout(
    const std::string& vertexFormat);

/// "front"/"none"/"back" (default) as an SDL_GPUCullMode.
SDL_GPUCullMode ResolveCullMode(const std::string& cullMode);

/// "d24_unorm_s8" or "d32_float" (default) as an SDL_GPUTextureFormat.
SDL_GPUTextureFormat ResolveDepthFormat(const std::string& depthFormat);

/**
 * @brief Resolves a named color target format.
 *
 * "rgba16_float", "r8_unorm", "b8g8r8a8_unorm" resolve directly; the
 * default "swapchain" queries the window's swapchain format, falling back
 * to B8G8R8A8_UNORM if no window is available.
 */
SDL_GPUTextureFormat ResolveColorTargetFormat(const std::string& colorFormat,
                                              SDL_GPUDevice* device,
                                              SDL_Window* window);

/// Enables standard src-alpha/one-minus-src-alpha blending on `target`.
void ApplyAlphaBlendState(SDL_GPUColorTargetDescription& target);

/**
 * @brief Assembles the full graphics-pipeline create-info from `p` and the
 *        already-resolved shaders/device/window.
 *
 * `layoutOut` and `colorTargetOut` are owned by the caller and must stay
 * alive until the returned info is passed to pipeline creation, since the
 * info's vertex/color-target fields point into them.
 */
SDL_GPUGraphicsPipelineCreateInfo BuildGraphicsPipelineCreateInfo(
    const GpuPipelineCreateParams& p, SDL_GPUShader* vertexShader,
    SDL_GPUShader* fragmentShader, SDL_GPUDevice* device, SDL_Window* window,
    GpuVertexAttributeLayout& layoutOut,
    SDL_GPUColorTargetDescription& colorTargetOut);

/// The precompiled vertex/fragment shaders a graphics pipeline is built
/// from.
struct GpuPipelineShaders {
    SDL_GPUShader* vertex   = nullptr;
    SDL_GPUShader* fragment = nullptr;
};

/**
 * @brief Looks up the vertex/fragment shaders `p` names in `context`.
 * @throws std::runtime_error (naming the missing key) if either is absent.
 */
GpuPipelineShaders RequireGpuPipelineShaders(WorkflowContext& context,
                                             const GpuPipelineCreateParams& p);

}  // namespace sdl3cpp::services::impl
