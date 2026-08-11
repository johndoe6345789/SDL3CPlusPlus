#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_definition.hpp"

namespace sdl3cpp::services {

class IWorkflowExecutor {
public:
    virtual ~IWorkflowExecutor() = default;

    virtual void Execute(const WorkflowDefinition& workflow, WorkflowContext& context) = 0;
};

}  // namespace sdl3cpp::services
