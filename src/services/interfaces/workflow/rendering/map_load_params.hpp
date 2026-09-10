#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// map.load's resolved parameters: the model file to import, a uniform
/// scale applied to every vertex, and whether to build a static physics
/// body per mesh from its bounding box.
struct MapLoadParams {
    std::string filePath;
    float scale        = 1.0f;
    bool createPhysics = true;
};

/// Reads `file_path`/`scale`/`create_physics`, falling back from a step
/// parameter to a wired context input (by `step.inputs`) for `file_path`,
/// exactly as map.load always has.
MapLoadParams ReadMapLoadParams(const WorkflowStepDefinition& step,
                                const WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
