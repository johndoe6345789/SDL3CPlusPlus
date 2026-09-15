#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.hud
 *
 * Draws the ioq3 status bar (ammo/health/armor + icons) each frame. See
 * q3_hud_draw.hpp for the layout/drawing logic; this step gathers
 * context state into a Q3HudAssets and publishes the resulting face-rect
 * for q3.hud_head_render's GPU blit step.
 */
class WorkflowQ3HudStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3HudStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
