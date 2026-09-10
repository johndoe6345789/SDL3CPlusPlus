#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <memory>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// Clamps `value` to [-1, 1], then rescales anything past `deadzone` back
/// onto the full [-1, 1] range (0 for anything within the deadzone).
float ApplyAxisDeadzone(float value, float deadzone);

/// Loads input.axis.combine's aggregation config: prefers
/// `input.aggregation.config` from the context, else reads the
/// `config_path` parameter (default
/// "packages/seed/workflows/input_aggregation.json") from disk. Throws
/// std::runtime_error if the file can't be opened.
nlohmann::json LoadInputAggregationConfig(const WorkflowStepDefinition& step,
                                          const WorkflowContext& context);

/// Reads one source ("key"/"mouse"/"gamepad_axis") from the context's
/// polled input state, exactly as input.axis.combine always has; unknown
/// source types read as 0.
float ReadAxisSourceValue(const nlohmann::json& source,
                          const WorkflowContext& context,
                          const nlohmann::json* keyState,
                          bool gamepadConnected);

/**
 * @brief Combines one axis binding's sources into a single [-1, 1] value
 * and writes it to every key in `axisBinding["outputs"]`.
 *
 * Each source's value is inverted/deadzoned/scaled before being summed;
 * the total is then clamped to [-1, 1]. Logs the result at Debug level
 * (via `logger`, which may be null) as `axisName`.
 */
void CombineAndWriteAxis(const std::string& axisName,
                         const nlohmann::json& axisBinding,
                         WorkflowContext& context,
                         const nlohmann::json* keyState,
                         bool gamepadConnected,
                         const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
