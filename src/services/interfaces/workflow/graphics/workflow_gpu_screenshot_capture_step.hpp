#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Saves the GPU swapchain to a BMP when the composite pass was skipped.
 *
 * This is the fallback capture path: it only applies when the postfx resources
 * were missing, so the screenshot still shows the 3D scene rather than a bare
 * CPU overlay.  Submits and drains the command buffer itself (the download has
 * to complete before the pixels can be read), then removes it from the context
 * so `gpu.command_buffer_submit` does not submit it twice.
 */
class WorkflowGpuScreenshotCaptureStep final : public IWorkflowStep {
public:
    explicit WorkflowGpuScreenshotCaptureStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
