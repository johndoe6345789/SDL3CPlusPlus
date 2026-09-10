#pragma once

#include "services/interfaces/workflow/rendering/box_face_context.hpp"

namespace sdl3cpp::services::impl {

/**
 * @brief Looks up the render pass/command buffer/pipeline, the unit-plane
 *        mesh, and `params.texture`'s GPU texture/sampler.
 *
 * Fills `out.pass`, `out.cmd`, `out.pipeline`, `out.vb`, `out.ib`,
 * `out.indexCount`, `out.texture` and `out.sampler`.
 *
 * @return true on success; false (after logging a warning) if any of those
 *         resources is missing from `context`.
 */
bool ResolveTexturedBoxResources(WorkflowContext& context, ILogger* logger,
                                 const DrawTexturedBoxParams& params,
                                 TexturedBoxDrawContext& out);

}  // namespace sdl3cpp::services::impl
