#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>

#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief GPU resources for blitting the SDL software-rendered overlay surface
 *        (and the 3D head portrait on top of it) onto the swapchain.
 *
 * Owned by `overlay.sw.end_init`, which publishes a pointer to it for the
 * `overlay.sw.end_*` steps that follow.  Unlike the FPS text overlay, this
 * pipeline is sized to the whole SW overlay surface and supports both Vulkan
 * (SPIR-V) and Metal (MSL) shader formats, chosen by the caller.
 */
struct OverlaySwEndResources {
    SDL_GPUDevice* device             = nullptr;
    SDL_GPUGraphicsPipeline* pipeline = nullptr;
    SDL_GPUTexture* texture           = nullptr;
    SDL_GPUTransferBuffer* transfer   = nullptr;
    SDL_GPUBuffer* vertices           = nullptr;
    SDL_GPUSampler* sampler           = nullptr;

    /// Lazily created the first time a head portrait is blitted; reuses
    /// `pipeline`, since the head quad has the same vertex layout.
    SDL_GPUBuffer* headVertices = nullptr;
    SDL_GPUSampler* headSampler = nullptr;

    bool vertexBufferUploaded = false;
};

/**
 * @brief Creates the pipeline, texture, buffers and sampler for the overlay.
 *
 * @param vertPath/fragPath Shader binary paths, already resolved to the
 *                           device's active backend (SPIR-V or MSL).
 * @return true if every resource was created; on false, whatever was created
 *         is already released and `out` is left default-constructed.
 */
bool CreateOverlaySwEndResources(SDL_GPUDevice* device, SDL_Window* window,
                                 int surfaceWidth, int surfaceHeight,
                                 const std::string& vertPath,
                                 const std::string& fragPath,
                                 OverlaySwEndResources& out);

/// Releases everything CreateOverlaySwEndResources() allocated and nulls it.
void DestroyOverlaySwEndResources(OverlaySwEndResources& res);

/**
 * @brief Picks the shader pair matching the GPU device's backend.
 *
 * Reads vert_shader_path_msl/frag_shader_path_msl when the device is Metal,
 * otherwise vert_shader_path_spirv/frag_shader_path_spirv; both fall back to
 * the "Q3 Overlay" sub-workflow's default packages/quake3/shaders paths.
 */
void ResolveOverlaySwEndShaderPaths(const WorkflowStepDefinition& step,
                                    SDL_GPUDevice* device,
                                    std::string& vertPath,
                                    std::string& fragPath);

}  // namespace sdl3cpp::services::impl
