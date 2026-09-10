#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow/rendering/gpu_text_overlay_resources.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Creates the FPS overlay's GPU resources on the first frame that has a
 *        GPU device, and publishes them for `postfx.overlay_fps_draw`.
 *
 * Idempotent: after the first successful (or permanently failed) attempt every
 * later execution is a no-op.  This step owns the resources for the lifetime of
 * the process and releases them in its destructor.
 */
class WorkflowPostfxOverlayFpsInitStep final : public IWorkflowStep {
public:
    explicit WorkflowPostfxOverlayFpsInitStep(std::shared_ptr<ILogger> logger);
    ~WorkflowPostfxOverlayFpsInitStep() override;

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    GpuTextOverlayResources resources_;
    bool attempted_ = false;
};

}  // namespace sdl3cpp::services::impl
