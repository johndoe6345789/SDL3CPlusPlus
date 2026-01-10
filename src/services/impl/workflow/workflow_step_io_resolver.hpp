#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

namespace sdl3cpp::services::impl {

class WorkflowStepIoResolver {
public:
    std::string GetRequiredInputKey(const WorkflowStepDefinition& step, const std::string& name) const;
    std::string GetRequiredOutputKey(const WorkflowStepDefinition& step, const std::string& name) const;
};

}  // namespace sdl3cpp::services::impl
