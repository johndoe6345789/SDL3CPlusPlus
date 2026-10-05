#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// A string parameter, else the environment variable `envName` (when
/// given), else `fallback`.
std::string RacerStringParam(const WorkflowStepDefinition& step,
                             const std::string& name, const char* envName,
                             const std::string& fallback);

/// A numeric parameter, else `fallback`.
int RacerIntParam(const WorkflowStepDefinition& step,
                  const std::string& name, int fallback);

}  // namespace sdl3cpp::services::impl
