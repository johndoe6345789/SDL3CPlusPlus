#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <nlohmann/json.hpp>
#include <cstdint>

namespace sdl3cpp::services::impl {

/// Layout/rendering parameters for one render.grid.draw frame, read
/// from the grid.config populated by render.grid.setup.
struct GridDrawConfig {
    uint32_t gridWidth  = 11u;
    uint32_t gridHeight = 11u;
    float spacing       = 3.0f;
    float startX        = -15.0f;
    float startY        = -15.0f;
    float rotOffsetX    = 0.21f;
    float rotOffsetY    = 0.37f;
    float bgR           = 0.18f;
    float bgG           = 0.18f;
    float bgB           = 0.18f;
    uint32_t numFrames  = 600u;
};

/// GPU handles the draw call needs, all borrowed from the context.
struct GridGpuResources {
    SDL_GPUDevice* device                  = nullptr;
    SDL_Window* window                     = nullptr;
    SDL_GPUGraphicsPipeline* pipeline      = nullptr;
    SDL_GPUBuffer* vertexBuffer            = nullptr;
    SDL_GPUBuffer* indexBuffer             = nullptr;
    SDL_GPUTexture* depthTexture           = nullptr;

    /// True only if every handle above was populated.
    bool IsComplete() const;
};

GridDrawConfig ReadGridDrawConfig(const nlohmann::json& cfg);

/// Reads the view/projection matrices from the camera named by
/// `grid.camera_key` (default "camera.state"); throws if missing or
/// malformed, matching the original step's behavior.
void ReadGridCameraMatrices(const WorkflowContext& context, glm::mat4& view,
                            glm::mat4& proj);

GridGpuResources ReadGridGpuResources(const WorkflowContext& context);

/// Runs one render pass over the cube grid: acquires a command buffer
/// and swapchain texture, clears, binds buffers, and issues one
/// indexed draw call per cube with its own MVP uniform. Returns the
/// number of draw calls actually submitted (0 if the swapchain/pass
/// could not be acquired).
uint32_t DrawGridCubes(const GridGpuResources& gpu,
                       const GridDrawConfig& cfg, const glm::mat4& view,
                       const glm::mat4& proj, float time);

}  // namespace sdl3cpp::services::impl
