#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow/rendering/gpu_text_overlay_resources.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: overlay.fps_init
 *
 * Creates the FPS overlay's GPU resources on the first frame that has a GPU
 * device, and publishes them for the `overlay.fps_upload_quad`,
 * `overlay.fps_upload_text` and `overlay.fps_draw` steps that follow.
 * Idempotent: after the first attempt (successful or not) later calls are a
 * cheap re-publish.  Also writes `overlay_fps_ready` and `overlay_fps_surface`
 * for `debug.screenshot` to read.
 *
 * Unlike `postfx.overlay_fps_init`, this reads `gpu_swapchain_texture` rather
 * than `postfx_swapchain_texture` — it is the standalone "Q3 Overlay"
 * sub-workflow's copy of the same overlay, drawn directly onto the raw
 * swapchain instead of on top of a postfx composite pass.
 */
class WorkflowOverlayFpsInitStep final : public IWorkflowStep {
public:
    explicit WorkflowOverlayFpsInitStep(std::shared_ptr<ILogger> logger);
    ~WorkflowOverlayFpsInitStep() override;

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    GpuTextOverlayResources resources_;
    bool attempted_ = false;
};

}  // namespace sdl3cpp::services::impl
