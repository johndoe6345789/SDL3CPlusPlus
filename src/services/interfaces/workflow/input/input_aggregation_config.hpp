#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// Loads input.axis.combine's aggregation config: prefers
/// `input.aggregation.config` from the context, else reads the
/// `config_path` parameter (default
/// "packages/seed/workflows/input_aggregation.json") from disk. Throws
/// std::runtime_error if the file can't be opened.
nlohmann::json LoadInputAggregationConfig(const WorkflowStepDefinition& step,
                                          const WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
