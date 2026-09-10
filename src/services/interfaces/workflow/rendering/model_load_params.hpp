#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// model.load's parsed parameters.
struct ModelLoadParams {
    std::string filePath;
    std::string name = "model";
    float scale      = 1.0f;
};

/// Reads `file_path`/`name`/`scale`, falling back from a step parameter
/// to the context key named by the matching `inputs` entry (file_path
/// and name only), then to the default.
ModelLoadParams ReadModelLoadParams(const WorkflowStepDefinition& step,
                                    const WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
