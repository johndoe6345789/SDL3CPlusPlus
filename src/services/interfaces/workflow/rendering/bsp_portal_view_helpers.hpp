#pragma once

#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// True and `out` set to the first trigger_teleport entity's
/// `target_position` found in `entities`; false if none has one.
bool FindPortalDestination(const nlohmann::json& entities, glm::vec3& out);

/// bsp.portal_view's lazily-created 512x512 render target, matching depth
/// buffer, and clamp/linear sampler, cached in the context under the
/// `bsp_portal_view_*` keys so later frames reuse them.
struct PortalViewTargets {
    SDL_GPUTexture* colorTex   = nullptr;
    SDL_GPUTexture* depthTex   = nullptr;
    SDL_GPUSampler* sampler    = nullptr;
};

/// Returns the cached targets, creating and caching any that are missing.
/// Any member left null means creation failed for that resource.
PortalViewTargets EnsurePortalViewTargets(SDL_GPUDevice* device,
                                          SDL_Window* window,
                                          WorkflowContext& context);

/// Builds the vertex/fragment uniforms for rendering the scene as seen from
/// `dest` looking along the player's current yaw/pitch (a 90-degree FOV
/// perspective, matching bsp.portal_view's original teleporter-preview look).
void BuildPortalViewUniforms(const WorkflowContext& context,
                             const glm::vec3& dest,
                             rendering::VertexUniformData& vu,
                             rendering::FragmentUniformData& fu);

/**
 * @brief Draws every BSP texture group in `mapNodes` into the portal pass.
 *
 * Binds the shared VB/IB from the first node, then per group binds its
 * albedo (skipping groups with none) plus the shared lightmap sampler, and
 * issues one indexed draw call. Returns false (having bound nothing) if the
 * first node's buffers are missing.
 */
bool DrawPortalViewGeometry(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                            WorkflowContext& context,
                            const nlohmann::json& mapNodes,
                            SDL_GPUGraphicsPipeline* pipeline,
                            SDL_GPUTexture* lmTex, SDL_GPUSampler* lmSamp,
                            const rendering::VertexUniformData& vu,
                            const rendering::FragmentUniformData& fu);

}  // namespace sdl3cpp::services::impl
