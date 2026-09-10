#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: overlay.sw.end_screenshot
 *
 * Saves the SW overlay surface to a BMP when `screenshot_output_path` is set.
 * This is a CPU-side capture of the 2D overlay only (HUD/crosshair/menu), not
 * the composited 3D scene — see gpu.screenshot_capture for that.
 */
class WorkflowOverlaySwEndScreenshotStep final : public IWorkflowStep {
public:
    explicit WorkflowOverlaySwEndScreenshotStep(
        std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
