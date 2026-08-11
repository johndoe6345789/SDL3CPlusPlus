#include "services/interfaces/workflow/workflow_generic_steps/workflow_try_catch_step.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowTryCatchStep::WorkflowTryCatchStep(std::shared_ptr<ILogger> logger,
                                           std::shared_ptr<IWorkflowStepRegistry> registry)
    : logger_(std::move(logger)),
      registry_(std::move(registry)) {
    if (!registry_) {
        throw std::runtime_error("WorkflowTryCatchStep requires a step registry");
    }
}

std::string WorkflowTryCatchStep::GetPluginId() const {
    return "control.try.catch";
}

void WorkflowTryCatchStep::Execute(const WorkflowStepDefinition& step,
                                   WorkflowContext& context) {
    // Get try step ID
    const auto tryStepIt = step.inputs.find("try_step");
    if (tryStepIt == step.inputs.end()) {
        throw std::runtime_error("control.try.catch requires 'try_step' input");
    }
    const std::string& tryStepId = tryStepIt->second;

    // Get catch step ID (optional)
    const auto catchStepIt = step.inputs.find("catch_step");
    const std::string catchStepId = catchStepIt != step.inputs.end() ? catchStepIt->second : "";

    // Get error output key (optional)
    const auto errorOutputIt = step.inputs.find("error_output");
    const std::string errorOutputKey = errorOutputIt != step.inputs.end() ? errorOutputIt->second : "error.message";

    // Execute try step
    bool error_occurred = false;
    std::string error_message;

    try {
        auto tryHandler = registry_->GetStep(tryStepId);
        if (!tryHandler) {
            throw std::runtime_error("control.try.catch: try step '" + tryStepId + "' not found");
        }

        // Create minimal step definition for try block
        WorkflowStepDefinition tryStep;
        tryStep.plugin = tryStepId;
        tryStep.id = tryStepId;

        tryHandler->Execute(tryStep, context);

        if (logger_) {
            logger_->Trace("WorkflowTryCatchStep", "Execute", "try_step=" + tryStepId,
                           "Try step executed successfully");
        }
    } catch (const std::exception& e) {
        error_occurred = true;
        error_message = e.what();

        // Store error message in context
        context.Set(errorOutputKey, error_message);

        if (logger_) {
            logger_->Trace("WorkflowTryCatchStep", "Execute",
                           "try_step=" + tryStepId + ", error=" + error_message,
                           "Exception caught");
        }

        // Execute catch step if provided and error occurred
        if (!catchStepId.empty()) {
            try {
                auto catchHandler = registry_->GetStep(catchStepId);
                if (!catchHandler) {
                    throw std::runtime_error("control.try.catch: catch step '" + catchStepId + "' not found");
                }

                // Create minimal step definition for catch block
                WorkflowStepDefinition catchStep;
                catchStep.plugin = catchStepId;
                catchStep.id = catchStepId;

                catchHandler->Execute(catchStep, context);

                if (logger_) {
                    logger_->Trace("WorkflowTryCatchStep", "Execute", "catch_step=" + catchStepId,
                                   "Catch step executed");
                }
            } catch (const std::exception& catchError) {
                if (logger_) {
                    logger_->Trace("WorkflowTryCatchStep", "Execute",
                                   "catch_step=" + catchStepId + ", error=" + std::string(catchError.what()),
                                   "Catch step threw exception");
                }
                throw;
            }
        }
    }
}

}  // namespace sdl3cpp::services::impl
