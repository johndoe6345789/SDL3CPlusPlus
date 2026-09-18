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

/// Like Bl4NumberOr, but a sibling `<name>_env` string parameter wins
/// when it holds a number. That is how a run overrides one of these
/// without editing the workflow: the JSON passes `${env:BL4_...}`, which
/// expands to "" (so, the JSON's own value) when the variable is unset.
float Bl4NumberOrEnv(const WorkflowStepDefinition& step, const std::string& name,
                     float fallback);

}  // namespace sdl3cpp::services::impl
