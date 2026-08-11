#include "services/interfaces/workflow/workflow_executor.hpp"

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
    // Only log step-by-step for the top-level workflow (not sub-workflows in frame loops)
    const bool verbose = !context.TryGet<bool>("_in_frame_loop");

    if (logger_ && verbose) {
        logger_->Trace("WorkflowExecutor", "Execute",
                       "steps=" + std::to_string(workflow.steps.size()),
                       "Starting workflow execution");
    }
    for (size_t i = 0; i < workflow.steps.size(); ++i) {
        const auto& step = workflow.steps[i];
        auto handler = registry_->GetStep(step.plugin);
        if (!handler) {
            if (logger_) {
                logger_->Warn("WorkflowExecutor: step " + std::to_string(i) + "/" + std::to_string(workflow.steps.size()) +
                             " skipping unregistered step '" + step.plugin + "' (id=" + step.id + ")");
            }
            continue;
        }
        if (logger_ && verbose) {
            logger_->Info("WorkflowExecutor: executing step " + std::to_string(i + 1) + "/" + std::to_string(workflow.steps.size()) +
                         " plugin='" + step.plugin + "' id='" + step.id + "'");
        }
        handler->Execute(step, context);
        if (logger_ && verbose) {
            logger_->Info("WorkflowExecutor: completed step '" + step.plugin + "' id='" + step.id + "'");
        }
    }
    if (logger_ && verbose) {
        logger_->Info("WorkflowExecutor: Workflow execution complete (" + std::to_string(workflow.steps.size()) + " steps)");
    }
}

}  // namespace sdl3cpp::services::impl
