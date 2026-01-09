#include "workflow_executor.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowExecutor::WorkflowExecutor(std::shared_ptr<IWorkflowStepRegistry> registry,
                                   std::shared_ptr<ILogger> logger)
    : registry_(std::move(registry)),
      logger_(std::move(logger)) {
    if (!registry_) {
        throw std::runtime_error("WorkflowExecutor requires a step registry");
    }
}

void WorkflowExecutor::Execute(const WorkflowDefinition& workflow, WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowExecutor", "Execute",
                       "steps=" + std::to_string(workflow.steps.size()),
                       "Starting workflow execution");
    }
    for (const auto& step : workflow.steps) {
        auto handler = registry_->GetStep(step.plugin);
        if (!handler) {
            throw std::runtime_error("WorkflowExecutor: no step registered for plugin '" + step.plugin + "'");
        }
        handler->Execute(step, context);
    }
    if (logger_) {
        logger_->Trace("WorkflowExecutor", "Execute", "", "Workflow execution complete");
    }
}

}  // namespace sdl3cpp::services::impl
