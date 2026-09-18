#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: bl4.loading.text
 *
 * Shows what bl4.player.hold is waiting on ("LOADING 62%") in the
 * engine's text overlay, and hands the overlay back to the FPS counter
 * once the tiles are in -- so, unlike gta5.loading.text, it never
 * uploads a blank of its own.
 *
 * Runs after postfx.composite_draw and after
 * postfx.overlay_fps_upload_text, with postfx.overlay_fps_draw after it.
 * The overlay holds 20 characters of SDL's debug font.
 *
 * Reads: bl4.loading.text, postfx_overlay_resources
 */
class WorkflowBl4LoadingTextStep final : public IWorkflowStep {
public:
    explicit WorkflowBl4LoadingTextStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::string shown_;
};

}  // namespace sdl3cpp::services::impl
