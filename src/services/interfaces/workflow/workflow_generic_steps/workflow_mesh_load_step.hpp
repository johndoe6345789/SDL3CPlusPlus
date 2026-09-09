#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_mesh_service.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

class WorkflowMeshLoadStep final : public IWorkflowStep {
public:
    WorkflowMeshLoadStep(std::shared_ptr<IMeshService> meshService,
                         std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<IMeshService> meshService_;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
