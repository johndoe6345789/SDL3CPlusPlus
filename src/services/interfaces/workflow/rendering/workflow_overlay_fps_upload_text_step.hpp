#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow/rendering/fps_meter.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: overlay.fps_upload_text
 *
 * Measures the frame rate, rasterises it via the SDL software renderer, and
 * uploads it to the overlay texture.  Owns the frame-rate average, so it must
 * execute exactly once per frame.
 */
class WorkflowOverlayFpsUploadTextStep final : public IWorkflowStep {
public:
    explicit WorkflowOverlayFpsUploadTextStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    FpsMeter meter_;
};

}  // namespace sdl3cpp::services::impl
