#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <cstdint>
#include <memory>

namespace sdl3cpp::services::impl {

class WorkflowPhysicsStepStep final : public IWorkflowStep {
public:
    explicit WorkflowPhysicsStepStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::uint64_t last_tick_ns_ = 0;  // wall-clock anchor for real-dt computation
};

}  // namespace sdl3cpp::services::impl
