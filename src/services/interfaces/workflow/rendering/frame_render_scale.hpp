#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

namespace sdl3cpp::services::impl {

/// A frame step's render_scale, clamped to [0.25, 4]; 1 when absent.
/// Above 1 the scene target is larger than the window -- supersampling,
/// resolved down by the composite.
float ReadFrameRenderScale(const WorkflowStepDefinition& step);

}  // namespace sdl3cpp::services::impl
