#pragma once

#include "services/interfaces/workflow/input/input_button_combine_config.hpp"
#include "services/interfaces/workflow/input/input_button_combine_sources.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// Writes `pressed` to every string in buttonBinding["outputs"].
void WriteButtonOutputs(const nlohmann::json& buttonBinding, bool pressed,
                        WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
