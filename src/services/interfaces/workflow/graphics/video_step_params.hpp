#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

namespace sdl3cpp::services::impl {

// A step parameter, which the environment variable @p env overrides
// when it is set and not empty.

double VideoStepNumber(const WorkflowStepDefinition& step, const char* name,
                       const char* env, double fallback);

std::string VideoStepString(const WorkflowStepDefinition& step,
                            const char* name, const char* env,
                            const std::string& fallback);

/// A JSON true/false, or any non-zero number, is on.
bool VideoStepFlag(const WorkflowStepDefinition& step, const char* name,
                   const char* env, bool fallback = false);

}  // namespace sdl3cpp::services::impl
