#pragma once

#include "services/interfaces/camera_types.hpp"
#include "services/interfaces/i_config_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

class WorkflowCameraBuildViewStateStep final : public IWorkflowStep {
public:
    WorkflowCameraBuildViewStateStep(std::shared_ptr<IConfigService> configService,
                                     std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<IConfigService> configService_;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
