#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.loading.text
 *
 * Shows what gta5.player.hold is waiting on -- indexing, reading, then
 * the spawn district's percentage -- in the engine's text overlay, and
 * takes it down once the ground is in by uploading a blank. Uploads only
 * when the text changes.
 *
 * Runs after postfx.composite_draw, with postfx.overlay_fps_init and
 * postfx.overlay_fps_upload_quad before it and postfx.overlay_fps_draw
 * after. The overlay holds 20 characters of SDL's debug font.
 *
 * Reads: gta5.loading.text, postfx_overlay_resources
 */
class WorkflowGta5LoadingTextStep final : public IWorkflowStep {
public:
    explicit WorkflowGta5LoadingTextStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::string shown_;
    bool uploaded_{false};
};

}  // namespace sdl3cpp::services::impl
