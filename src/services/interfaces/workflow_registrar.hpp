#pragma once

#include "services/interfaces/i_workflow_step_registry.hpp"
#include "services/interfaces/i_workflow_executor.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

class WorkflowRegistrar {
public:
    explicit WorkflowRegistrar(std::shared_ptr<ILogger> logger);
    void RegisterSteps(std::shared_ptr<IWorkflowStepRegistry> registry);
    void RegisterExecutorSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                               std::shared_ptr<IWorkflowExecutor> executor);

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
