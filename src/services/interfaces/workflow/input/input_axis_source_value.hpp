#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// Reads one source ("key"/"mouse"/"gamepad_axis") from the context's
/// polled input state, exactly as input.axis.combine always has; unknown
/// source types read as 0.
float ReadAxisSourceValue(const nlohmann::json& source,
                          const WorkflowContext& context,
                          const nlohmann::json* keyState,
                          bool gamepadConnected);

}  // namespace sdl3cpp::services::impl
