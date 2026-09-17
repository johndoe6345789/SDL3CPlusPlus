#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// Read a string parameter, falling back when absent or not a string.
std::string Bl4StringOr(const WorkflowStepDefinition& step,
                        const std::string& name, const std::string& fallback);

/// Read a numeric parameter as a float, falling back when absent.
float Bl4NumberOr(const WorkflowStepDefinition& step, const std::string& name,
                  float fallback);

}  // namespace sdl3cpp::services::impl
