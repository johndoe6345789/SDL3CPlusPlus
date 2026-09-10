#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/quake3/q3_pickup_render.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

class WorkflowQ3PickupsDrawStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3PickupsDrawStep(std::shared_ptr<ILogger> logger);
    ~WorkflowQ3PickupsDrawStep();

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    PickupQuadBuffers quadBuffers_;
};

}  // namespace sdl3cpp::services::impl
