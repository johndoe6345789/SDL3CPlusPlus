#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

class WorkflowPhysicsFpsMoveStep final : public IWorkflowStep {
public:
    explicit WorkflowPhysicsFpsMoveStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    // Accumulator that gates step-up to ~60Hz so high-refresh frames don't
    // multi-teleport the player up a single staircase.
    float step_accumulator_s_ = 0.0f;
    float jam_time_s_         = 0.0f;
};

}  // namespace sdl3cpp::services::impl
