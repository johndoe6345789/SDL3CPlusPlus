#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>

#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Loads input.button.combine's aggregation config.
 *
 * Prefers the "input.aggregation.config" context key if it holds a JSON
 * object; otherwise reads it from `configPath`. Throws
 * std::runtime_error if the file can't be opened.
 */
nlohmann::json LoadButtonAggregationConfig(const WorkflowContext& context,
                                           const std::string& configPath);

/**
 * @brief Evaluates one binding source ({"type": "key"|"mouse_button"|
 * "gamepad_button", ...}) against the current input state.
 *
 * `keyState` may be null (treated as "no keys held"). Unknown source
 * types, or gamepad sources while no gamepad is connected, read as not
 * pressed.
 */
bool IsSourcePressed(const nlohmann::json& source,
                     const nlohmann::json* keyState, bool gamepadConnected,
                     const WorkflowContext& context);

/// Writes `pressed` to every string in buttonBinding["outputs"].
void WriteButtonOutputs(const nlohmann::json& buttonBinding, bool pressed,
                        WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
