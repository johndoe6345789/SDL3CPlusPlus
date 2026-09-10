#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <string>

namespace sdl3cpp::services::impl {

/// The single shared unit quad (and its upload transfer buffer) every
/// pickup billboard is drawn from. Owned by the step instance so it is
/// created once and released once, across many frames.
struct PickupQuadBuffers {
    SDL_GPUDevice* device            = nullptr;
    SDL_GPUBuffer* quadVb            = nullptr;
    SDL_GPUBuffer* quadIb            = nullptr;
    SDL_GPUTransferBuffer* transfer  = nullptr;
};

/// Creates `buffers`' vertex/index buffers on first call; a no-op once
/// both are already created.
void EnsurePickupQuadBuffers(SDL_GPUDevice* device,
                             PickupQuadBuffers& buffers);

/// Releases any GPU resources `buffers` owns. Safe to call unconditionally
/// (e.g. from a destructor) even if buffers were never created.
void ReleasePickupQuadBuffers(PickupQuadBuffers& buffers);

/**
 * @brief Returns the 1x1 solid-color GPU texture+sampler cached under
 *        `key` in `context`, creating and caching it on first use.
 *
 * The color is stored with alpha 230 (a soft translucency for pickup
 * billboards). Returns the existing texture if `key + "_gpu"` is already
 * set, ignoring `r`/`g`/`b` in that case.
 */
SDL_GPUTexture* EnsurePickupColorTexture(SDL_GPUDevice* device,
                                         WorkflowContext& context,
                                         const std::string& key, uint8_t r,
                                         uint8_t g, uint8_t b);

/**
 * @brief Draws a billboard quad for every uncollected pickup entity in
 *        `entities`, textured by pickup category.
 *
 * Skips entities that aren't a recognized pickup classname, are already
 * marked collected in `collected`, or lack a valid "position". Caps at 96
 * draws per call. `time` drives the vertical bob animation.
 */
void DrawPickupEntities(const nlohmann::json& entities,
                        const nlohmann::json& collected,
                        const glm::mat4& view, const glm::mat4& proj,
                        const glm::vec3& camPos, const glm::mat4& shadowVP,
                        float time, SDL_GPURenderPass* pass,
                        SDL_GPUCommandBuffer* cmd,
                        const PickupQuadBuffers& buffers,
                        WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
