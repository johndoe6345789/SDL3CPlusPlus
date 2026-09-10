#pragma once

#include "services/interfaces/workflow/quake3/q3_md3_draw_params.hpp"
#include "services/interfaces/workflow_context.hpp"

namespace sdl3cpp::services::impl {

/// Picks the frame to draw: `frameKey` if set, else an fps/animation-range
/// derived frame from `frame.elapsed`, clamped to [0, numFrames - 1].
int ResolveMd3AnimFrame(const WorkflowContext& context,
                        const Md3DrawParams& params, int numFrames);

}  // namespace sdl3cpp::services::impl
