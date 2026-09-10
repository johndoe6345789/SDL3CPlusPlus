#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/rendering/box_face_geometry.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <cstdint>

namespace sdl3cpp::services::impl {

/// GPU state and per-draw values resolved from `context` for one
/// `draw.textured_box` call.
struct TexturedBoxDrawContext {
    SDL_GPURenderPass* pass           = nullptr;
    SDL_GPUCommandBuffer* cmd         = nullptr;
    SDL_GPUGraphicsPipeline* pipeline = nullptr;
    SDL_GPUBuffer* vb                 = nullptr;
    SDL_GPUBuffer* ib                 = nullptr;
    uint32_t indexCount               = 0;
    SDL_GPUTexture* texture           = nullptr;
    SDL_GPUSampler* sampler           = nullptr;
    glm::mat4 view{1.0f};
    glm::mat4 proj{1.0f};
    glm::vec3 camPos{0.0f};
    glm::vec3 center{0.0f};
    glm::mat4 bodyRotation{1.0f};
    rendering::FragmentUniformData fu{};
};

/**
 * @brief Looks up the GPU render state, unit-plane mesh, texture, camera,
 *        and lighting that a `draw.textured_box` call needs.
 *
 * When `params.body` is set, also pulls that body's synced transform (from
 * the physics.sync_transforms step) to override `out.center`/
 * `out.bodyRotation`.
 *
 * @return true with `out` fully populated; false (after logging a warning)
 *         if a required resource — the render pass/command buffer/pipeline,
 *         the unit plane, or the texture — is missing from `context`.
 */
bool ResolveTexturedBoxDraw(WorkflowContext& context, ILogger* logger,
                            const DrawTexturedBoxParams& params,
                            TexturedBoxDrawContext& out);

}  // namespace sdl3cpp::services::impl
