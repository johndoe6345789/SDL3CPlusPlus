#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// Read a string parameter, falling back when absent or not a string.
std::string Gta5ParameterOr(const WorkflowStepDefinition& step,
                            const std::string& name,
                            const std::string& fallback);

/// Read a numeric parameter as an int, falling back when absent.
int Gta5ParameterOrInt(const WorkflowStepDefinition& step,
                       const std::string& name, int fallback);

}  // namespace sdl3cpp::services::impl
