#pragma once

#include "services/interfaces/workflow/quake3/q3_hud_head_draw.hpp"
#include "services/interfaces/workflow/quake3/q3_hud_head_sway.hpp"
#include "services/interfaces/workflow/quake3/q3_hud_head_targets.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/**
 * @brief Opens an offscreen pass on `targets`, draws the head, and ends it.
 *
 * @param size Render target width/height (square) used for the viewport.
 * @return false if the render pass could not be opened, in which case the
 *         caller should treat the frame as having no portrait to show.
 */
bool RenderHeadPortraitPass(SDL_GPUCommandBuffer* cmd,
                            const HeadRenderTargets& targets,
                            SDL_GPUGraphicsPipeline* pipeline,
                            const glm::mat4& mvp, const glm::mat4& model,
                            const rendering::FragmentUniformData& fu,
                            WorkflowContext& context, int size);

}  // namespace sdl3cpp::services::impl
