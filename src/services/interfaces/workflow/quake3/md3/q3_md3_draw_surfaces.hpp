#pragma once

#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Draws every uploaded surface of a named MD3 at the given model
 * matrix, for the given animation frame.
 *
 * Reads each surface's GPU buffers/textures from context under the
 * `q3.md3.<prefix>_surf<N>_*` keys populated by the MD3 load/upload
 * steps. Surfaces missing their buffers or textures are skipped.
 *
 * Always binds 2 fragment samplers -- shader slot 1 is the shadow map.
 * When no shadow texture is available, the albedo texture/sampler is
 * reused for slot 1 so it is never null (a Metal validation error); the
 * shadow UV will then be out of bounds (shadow_vp is identity) so
 * ComputeShadowPCF returns 1.0 (no shadow).
 */
void DrawMd3Surfaces(const std::string& prefix, int frame,
                     const glm::mat4& model, const glm::mat4& view,
                     const glm::mat4& proj, const glm::vec3& camPos,
                     const glm::mat4& shadowVP,
                     const rendering::FragmentUniformData& fu,
                     SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                     SDL_GPUTexture* shadowTex, SDL_GPUSampler* shadowSamp,
                     WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
