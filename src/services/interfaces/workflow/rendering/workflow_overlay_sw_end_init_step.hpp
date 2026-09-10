#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow/rendering/overlay_sw_end_resources.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: overlay.sw.end_init
 *
 * Presents the SDL software renderer (flushing its batch to the overlay
 * surface's pixels) and, on the first frame a GPU device and that surface are
 * both available, creates the GPU resources the `overlay.sw.end_*` steps that
 * follow use to blit it onto the swapchain.  Idempotent after the first
 * attempt.  A no-op — and every later step in the family a no-op too — when
 * `overlay.ready` is false (the "Q3 Overlay" sub-workflow's SW overlay was
 * never begun this frame).
 *
 * Parameters: vert_shader_path_msl/spirv, frag_shader_path_msl/spirv — the
 *   pair actually used depends on the GPU device's backend.
 */
class WorkflowOverlaySwEndInitStep final : public IWorkflowStep {
public:
    explicit WorkflowOverlaySwEndInitStep(std::shared_ptr<ILogger> logger);
    ~WorkflowOverlaySwEndInitStep() override;

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    OverlaySwEndResources resources_;
    bool attempted_ = false;
};

}  // namespace sdl3cpp::services::impl
