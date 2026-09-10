#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Submits the frame's GPU command buffer and clears it from the context.
 *
 * A no-op when there is no command buffer, which is how it stays correct after
 * `gpu.screenshot_capture` has already submitted and removed one.
 */
class WorkflowGpuCommandBufferSubmitStep final : public IWorkflowStep {
public:
    explicit WorkflowGpuCommandBufferSubmitStep(
        std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
