#pragma once

#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <array>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Reads a 16-number "matrix" input/parameter, shared by
 * model.spawn and model.set_transform.
 *
 * Checked in order: the `matrix` input's context value (a 16-element
 * vector<double>), then the `matrix` parameter (a 16-element number
 * list). If neither is set: returns `*fallback` if non-null, else
 * throws std::runtime_error naming `stepName`.
 */
std::array<float, 16> ReadMatrixInput(
    const WorkflowStepDefinition& step, const WorkflowContext& context,
    const WorkflowStepParameterResolver& parameterResolver,
    const std::string& stepName, const std::array<float, 16>* fallback);

/**
 * @brief Reads an "object_type" input/parameter, shared by model.spawn
 * and model.set_transform.
 *
 * Checked in order: the `object_type` input's context value, then the
 * `object_type` parameter. If neither is set: returns `*fallback` if
 * non-null, else throws std::runtime_error naming `stepName`.
 */
std::string ReadObjectTypeInput(
    const WorkflowStepDefinition& step, const WorkflowContext& context,
    const WorkflowStepParameterResolver& parameterResolver,
    const std::string& stepName, const std::string* fallback);

}  // namespace sdl3cpp::services::impl
