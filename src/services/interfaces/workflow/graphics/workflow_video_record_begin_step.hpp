#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/// `video.record.begin`: on a frame due for the recording, redirects
/// postfx_swapchain_texture to a texture that can be read back.
class WorkflowVideoRecordBeginStep final : public IWorkflowStep {
public:
    explicit WorkflowVideoRecordBeginStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
