#pragma once

#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <array>
#include <string>

namespace sdl3cpp::services::impl {

/// Reads a 3-number field named `name` for camera.set_pose: prefers a
/// wired context input (a `std::vector<double>` of size 3), falls back to
/// a `NumberList` parameter of size 3, else returns `fallback`. Throws
/// std::runtime_error if either source is present but the wrong shape.
std::array<float, 3> ReadCameraPoseVec3(
    const WorkflowStepDefinition& step, const WorkflowContext& context,
    const WorkflowStepParameterResolver& parameterResolver,
    const std::string& name, const std::array<float, 3>& fallback);

/// Reads a single-number field named `name` for camera.set_pose: prefers a
/// wired context input (a `double`), falls back to a `Number` parameter,
/// else returns `fallback`. Throws std::runtime_error if either source is
/// present but not a number.
float ReadCameraPoseNumber(
    const WorkflowStepDefinition& step, const WorkflowContext& context,
    const WorkflowStepParameterResolver& parameterResolver,
    const std::string& name, float fallback);

}  // namespace sdl3cpp::services::impl
