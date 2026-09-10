#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// A freshly created shadow depth texture plus its sampler.
struct ShadowDepthTarget {
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
};

/**
 * @brief Creates a `map_size`x`map_size` D32_FLOAT depth texture and a
 *        nearest/clamp-to-edge sampler for shadow rendering.
 *
 * @throws std::runtime_error if either GPU resource fails to create.
 */
ShadowDepthTarget CreateShadowDepthTarget(SDL_GPUDevice* device, int map_size);

/**
 * @brief Computes the light view-projection matrix for the map's
 *        directional light (`context["lighting.directional"]`, or a
 *        straight-down light when absent).
 */
glm::mat4 ComputeShadowLightViewProjection(const WorkflowContext& context,
                                           float scene_extent, float near_plane,
                                           float far_plane);

}  // namespace sdl3cpp::services::impl
