#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// Reads a string parameter, falling back to `fallback`, then to the
/// environment variable `envName` when the parameter is absent.
std::string StuntsStringOr(const WorkflowStepDefinition& step,
                           const std::string& name, const char* envName,
                           const std::string& fallback);

/// Reads a numeric parameter, or `fallback` when it is absent.
float StuntsNumberOr(const WorkflowStepDefinition& step,
                     const std::string& name, float fallback);

}  // namespace sdl3cpp::services::impl
