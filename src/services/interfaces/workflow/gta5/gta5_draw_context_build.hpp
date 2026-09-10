#pragma once

#include "services/interfaces/workflow/gta5/gta5_draw_instances.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

namespace sdl3cpp::services::impl {

/// Gather everything one frame's instance draws need out of the workflow
/// context, so the draw step stays a thin plugin wrapper.
Gta5DrawContext BuildGta5DrawContext(const WorkflowStepDefinition& step,
                                     const WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
