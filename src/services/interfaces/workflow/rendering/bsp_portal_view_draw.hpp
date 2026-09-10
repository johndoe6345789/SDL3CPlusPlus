#pragma once

#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

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
