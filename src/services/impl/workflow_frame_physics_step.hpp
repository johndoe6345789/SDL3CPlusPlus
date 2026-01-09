#pragma once

#include "../interfaces/i_workflow_step.hpp"
#include "../interfaces/i_logger.hpp"
#include "../interfaces/i_physics_service.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

class WorkflowFramePhysicsStep final : public IWorkflowStep {
public:
    WorkflowFramePhysicsStep(std::shared_ptr<IPhysicsService> physicsService,
                             std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<IPhysicsService> physicsService_;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
