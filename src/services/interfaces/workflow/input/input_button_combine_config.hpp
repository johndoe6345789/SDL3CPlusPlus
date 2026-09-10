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

}  // namespace sdl3cpp::services::impl
