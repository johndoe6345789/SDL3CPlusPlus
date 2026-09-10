#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow/rendering/fps_meter.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Measures the frame rate, rasterises it, and uploads it to the
 *        overlay texture.
 *
 * Runs before `postfx.overlay_fps_draw` in the same command buffer.  Owns the
 * frame-rate average, so it must execute exactly once per frame.
 */
class WorkflowPostfxOverlayFpsUploadTextStep final : public IWorkflowStep {
public:
    explicit WorkflowPostfxOverlayFpsUploadTextStep(
        std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    FpsMeter meter_;
};

}  // namespace sdl3cpp::services::impl
