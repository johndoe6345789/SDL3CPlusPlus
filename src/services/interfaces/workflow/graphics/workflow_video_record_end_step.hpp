#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/// `video.record.end`: blits a recorded frame to the swapchain and
/// queues its download. Runs just before the command buffer submit.
class WorkflowVideoRecordEndStep final : public IWorkflowStep {
public:
    explicit WorkflowVideoRecordEndStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
