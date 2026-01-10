#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_probe_service.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

class WorkflowConfigSchemaStep : public IWorkflowStep {
public:
    WorkflowConfigSchemaStep(std::shared_ptr<ILogger> logger,
                             std::shared_ptr<IProbeService> probeService);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IProbeService> probeService_;
};

}  // namespace sdl3cpp::services::impl
