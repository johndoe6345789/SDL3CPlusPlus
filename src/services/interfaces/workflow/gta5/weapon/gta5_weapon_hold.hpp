#pragma once

#include "services/interfaces/workflow_context.hpp"

namespace sdl3cpp::services::impl {

/// The trigger: the left button, or the pad's right trigger.
bool Gta5TriggerHeld(WorkflowContext& context);

/// Down the sights: either mouse button, or either pad trigger. What is
/// merely carried hangs at the side, so this is what raises it.
bool Gta5AimHeld(WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
