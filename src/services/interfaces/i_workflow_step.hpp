#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

namespace sdl3cpp::services {

class IWorkflowStep {
public:
    virtual ~IWorkflowStep() = default;

    virtual std::string GetPluginId() const = 0;
    virtual void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) = 0;
};

}  // namespace sdl3cpp::services
