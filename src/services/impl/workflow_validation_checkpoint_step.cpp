#include "workflow_validation_checkpoint_step.hpp"
#include "workflow_step_io_resolver.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowValidationCheckpointStep::WorkflowValidationCheckpointStep(
    std::shared_ptr<IValidationTourService> validationService,
    std::shared_ptr<ILogger> logger)
    : validationService_(std::move(validationService)),
      logger_(std::move(logger)) {}

std::string WorkflowValidationCheckpointStep::GetPluginId() const {
    return "validation.tour.checkpoint";
}

void WorkflowValidationCheckpointStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!validationService_) {
        throw std::runtime_error("validation.tour.checkpoint requires an IValidationTourService");
    }
    WorkflowStepIoResolver resolver;
    const std::string checkpointKey = resolver.GetRequiredInputKey(step, "checkpoint");
    const auto* checkpointValue = context.TryGet<std::string>(checkpointKey);
    const std::string checkpointId = checkpointValue ? *checkpointValue : checkpointKey;
    const bool fromContext = checkpointValue != nullptr;
    if (checkpointId.empty()) {
        throw std::runtime_error("validation.tour.checkpoint missing checkpoint id");
    }
    if (!validationService_->RequestCheckpoint(checkpointId)) {
        throw std::runtime_error("validation.tour.checkpoint checkpoint not found: " + checkpointId);
    }
    if (logger_) {
        logger_->Trace("WorkflowValidationCheckpointStep", "Execute",
                       "checkpoint=" + checkpointId +
                           ", source=" + std::string(fromContext ? "context" : "literal") +
                           ", active=" + std::string(validationService_->IsActive() ? "true" : "false"),
                       "Checkpoint scheduled");
    }
}

}  // namespace sdl3cpp::services::impl
