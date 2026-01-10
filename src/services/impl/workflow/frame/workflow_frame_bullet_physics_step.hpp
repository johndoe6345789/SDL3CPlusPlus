#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_physics_service.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

class WorkflowFrameBulletPhysicsStep final : public IWorkflowStep {
public:
    WorkflowFrameBulletPhysicsStep(std::shared_ptr<IPhysicsService> physicsService,
                                   std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<IPhysicsService> physicsService_;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
