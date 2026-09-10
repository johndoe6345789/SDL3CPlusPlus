#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// GPU handles the draw call needs, all borrowed from the context.
struct GridGpuResources {
    SDL_GPUDevice* device             = nullptr;
    SDL_Window* window                = nullptr;
    SDL_GPUGraphicsPipeline* pipeline = nullptr;
    SDL_GPUBuffer* vertexBuffer       = nullptr;
    SDL_GPUBuffer* indexBuffer        = nullptr;
    SDL_GPUTexture* depthTexture      = nullptr;

    /// True only if every handle above was populated.
    bool IsComplete() const;
};

/// Reads the view/projection matrices from the camera named by
/// `grid.camera_key` (default "camera.state"); throws if missing or
/// malformed, matching the original step's behavior.
void ReadGridCameraMatrices(const WorkflowContext& context, glm::mat4& view,
                            glm::mat4& proj);

GridGpuResources ReadGridGpuResources(const WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
