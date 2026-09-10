#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

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

}  // namespace sdl3cpp::services::impl
