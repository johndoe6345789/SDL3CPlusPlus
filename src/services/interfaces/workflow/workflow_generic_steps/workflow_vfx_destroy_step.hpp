#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Workflow step to destroy/clean up visual effects (VFX)
 *
 * Plugin ID: vfx.destroy
 *
 * Parameters:
 *   - vfx_id (string): ID of effect to destroy
 *   - vfx_ids (string): Comma-separated list of IDs to destroy
 *   - effect_type (string): Destroy all effects of type
 *   - target (string): Special targets: "oldest", "newest", "all"
 *   - destroy_all (bool): Destroy all active effects
 *   - mode (string): Destruction mode: immediate, fade_out, animated (default: immediate)
 *   - fade_duration (number): Duration of fade-out in seconds (default: 1.0)
 *   - free_resources (bool): Release VFX resources (default: true)
 *   - free_textures (bool): Release texture resources (default: false)
 *
 * Outputs:
 *   - destroyed (bool): Whether effect was found and destroyed
 *   - remaining_count (number): Number of remaining active VFX
 */
class WorkflowVfxDestroyStep : public IWorkflowStep {
public:
    explicit WorkflowVfxDestroyStep(std::shared_ptr<ILogger> logger);
    ~WorkflowVfxDestroyStep() override = default;

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
