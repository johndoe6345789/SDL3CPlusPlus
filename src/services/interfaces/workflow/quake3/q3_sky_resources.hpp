#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>

namespace sdl3cpp::services::impl {

/// GPU resources for the sky dome, built once on the first drawn frame.
struct SkyResources {
    SDL_GPUTexture* cloudTex     = nullptr;
    SDL_GPUSampler* cloudSampler = nullptr;
    SDL_GPUTexture* whiteTex     = nullptr;
    SDL_GPUSampler* whiteSampler = nullptr;
    SDL_GPUBuffer* vertexBuffer  = nullptr;
    SDL_GPUBuffer* indexBuffer   = nullptr;
    uint32_t indexCount          = 0;
    /// Set on the first attempt, successful or not, so a map without a
    /// sky is not re-probed every frame.
    bool attempted = false;
};

/**
 * @brief Loads the map's sky cloud image and uploads the dome mesh.
 *
 * The sky shader is found by name among the BSP's textures (Quake keeps
 * them under textures/skies/) and its image comes from the shader
 * script's first stage, which for tim_hell is killsky_1.
 */
bool InitSkyResources(WorkflowContext& context, SkyResources& out);

}  // namespace sdl3cpp::services::impl
