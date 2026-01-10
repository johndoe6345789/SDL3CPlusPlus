#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_validation_tour_service.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

class WorkflowValidationCheckpointStep final : public IWorkflowStep {
public:
    WorkflowValidationCheckpointStep(std::shared_ptr<IValidationTourService> validationService,
                                     std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<IValidationTourService> validationService_;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
