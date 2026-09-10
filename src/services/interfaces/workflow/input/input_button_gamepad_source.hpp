#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>

#include <string>

namespace sdl3cpp::services::impl {

/// True if the config's named gamepad button/axis (e.g. "a", "lb",
/// "trigger_left") reads as pressed in the current input context.
/// `source` supplies the "threshold" for trigger axes (default 0.5).
bool IsGamepadButtonPressed(const std::string& btnStr,
                            const nlohmann::json& source,
                            const WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
