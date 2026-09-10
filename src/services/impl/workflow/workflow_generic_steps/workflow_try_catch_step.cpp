#include "services/interfaces/workflow/workflow_generic_steps/workflow_try_catch_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/try_catch_dispatch.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowTryCatchStep::WorkflowTryCatchStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IWorkflowStepRegistry> registry)
    : logger_(std::move(logger)), registry_(std::move(registry)) {
    if (!registry_) {
        throw std::runtime_error(
            "WorkflowTryCatchStep requires a step registry");
    }
}

std::string WorkflowTryCatchStep::GetPluginId() const {
    return "control.try.catch";
}

void WorkflowTryCatchStep::Execute(const WorkflowStepDefinition& step,
                                   WorkflowContext& context) {
    const auto tryStepIt = step.inputs.find("try_step");
    if (tryStepIt == step.inputs.end()) {
        throw std::runtime_error(
            "control.try.catch requires 'try_step' input");
    }
    const std::string& tryStepId = tryStepIt->second;

    const auto catchStepIt = step.inputs.find("catch_step");
    const std::string catchStepId =
        catchStepIt != step.inputs.end() ? catchStepIt->second : "";

    const auto errorOutputIt = step.inputs.find("error_output");
    const std::string errorOutputKey = errorOutputIt != step.inputs.end()
                                          ? errorOutputIt->second
                                          : "error.message";
    try {
        ExecuteRegisteredStep(*registry_, tryStepId, context, "try");
        if (logger_) {
            logger_->Trace("WorkflowTryCatchStep", "Execute",
                          "try_step=" + tryStepId,
                          "Try step executed successfully");
        }
    } catch (const std::exception& e) {
        const std::string error_message = e.what();
        context.Set(errorOutputKey, error_message);

        if (logger_) {
            logger_->Trace("WorkflowTryCatchStep", "Execute",
                          "try_step=" + tryStepId +
                              ", error=" + error_message,
                          "Exception caught");
        }

        if (catchStepId.empty()) return;
        try {
            ExecuteRegisteredStep(*registry_, catchStepId, context, "catch");
            if (logger_) {
                logger_->Trace("WorkflowTryCatchStep", "Execute",
                              "catch_step=" + catchStepId,
                              "Catch step executed");
            }
        } catch (const std::exception& catchError) {
            if (logger_) {
                logger_->Trace(
                    "WorkflowTryCatchStep", "Execute",
                    "catch_step=" + catchStepId +
                        ", error=" + std::string(catchError.what()),
                    "Catch step threw exception");
            }
            throw;
        }
    }
}

}  // namespace sdl3cpp::services::impl
