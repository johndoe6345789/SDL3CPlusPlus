#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.bots.draw
 *
 * Renders each bot as a three-part player model chain:
 *   lower.md3 (legs) at bot world position
 *   → upper.md3 (torso) attached via lower's tag_torso
 *   → head.md3  attached via upper's tag_head
 *   → weapon.md3 attached via upper's tag_weapon (optional)
 *
 * Parameters:
 *   lower_prefix   (string) – MD3 prefix for lower.md3 (default "lower")
 *   upper_prefix   (string) – MD3 prefix for upper.md3 (default "upper")
 *   head_prefix    (string) – MD3 prefix for head.md3  (default "head")
 *   weapon_prefix  (string) – MD3 prefix for weapon    (default "weapon_mg")
 *
 * Reads: q3.bots, gpu_render_pass, gpu_command_buffer, gpu_pipeline_textured,
 *        render.* matrices, q3.md3.{prefix}_* from q3.md3.load
 */
class WorkflowQ3BotsDrawStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3BotsDrawStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
