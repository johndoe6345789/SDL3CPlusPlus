#pragma once

#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Draws all surfaces of a named MD3 into an already-bound pass.
 *
 * `prefix` selects the MD3 previously uploaded to context under
 * `q3.md3.<prefix>_*` keys (see WorkflowQ3Md3UploadSurfacesStep).
 */
void DrawHeadMd3(const std::string& prefix, const glm::mat4& mvp,
                 const glm::mat4& model,
                 const rendering::FragmentUniformData& fu,
                 SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                 WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
