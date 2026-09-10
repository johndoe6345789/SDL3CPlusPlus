#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

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
                         const nlohmann::json* keyState, bool gamepadConnected,
                         const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
