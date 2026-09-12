#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// Read a string parameter, falling back when absent or not a string.
std::string Gta5ParameterOr(const WorkflowStepDefinition& step,
                            const std::string& name,
                            const std::string& fallback);

/// Read a numeric parameter as a float, falling back when absent.
float Gta5NumberOr(const WorkflowStepDefinition& step,
                   const std::string& name, float fallback);

/// Read a numeric parameter as an int, falling back when absent.
int Gta5ParameterOrInt(const WorkflowStepDefinition& step,
                       const std::string& name, int fallback);

/// Resolve a path parameter against the package directory.
///
/// package_dir is only set when a workflow runs app.init, which the
/// bootstrap packages do not. When it is absent the parameter is used as
/// written, which resolves against the working directory the app was
/// launched from -- the same place packages/ is synced to.
std::string Gta5ResolvePath(const WorkflowStepDefinition& step,
                            const WorkflowContext& context,
                            const std::string& name,
                            const std::string& fallback);

}  // namespace sdl3cpp::services::impl
