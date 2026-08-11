#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_platform_service.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

class WorkflowGraphicsGpuInitStep final : public IWorkflowStep {
public:
    explicit WorkflowGraphicsGpuInitStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<IPlatformService> platform_service
    );

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IPlatformService> platform_service_;
};

}  // namespace sdl3cpp::services::impl
