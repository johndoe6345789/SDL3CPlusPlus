#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: overlay.sw.end_blit_head
 *
 * Blits the 3D head portrait rendered by q3.hud_head_render over the HUD's
 * face-rect area, on top of the just-drawn SW overlay.  A no-op when no head
 * texture was produced this frame (e.g. the HUD head render is disabled).
 */
class WorkflowOverlaySwEndBlitHeadStep final : public IWorkflowStep {
public:
    explicit WorkflowOverlaySwEndBlitHeadStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
