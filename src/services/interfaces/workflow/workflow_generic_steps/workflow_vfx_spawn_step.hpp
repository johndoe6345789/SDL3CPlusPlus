#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Workflow step to spawn visual effects (VFX) at a location
 *
 * Plugin ID: vfx.spawn
 *
 * Parameters:
 *   - effect_type (string): Type of effect (explosion, smoke, fire, dust, etc)
 *   - position_x/y/z (number): Spawn position (default: 0,0,0)
 *   - rotation_x/y/z (number): Rotation in degrees (default: 0,0,0)
 *   - velocity_x/y/z (number): Initial velocity (default: 0,0,0)
 *   - scale (number): Uniform scale (default: 1.0)
 *   - scale_x/y/z (number): Non-uniform scale
 *   - color_r/g/b (number): RGB color 0-255 (default: 255,255,255)
 *   - intensity (number): Effect intensity (default: 1.0)
 *   - alpha (number): Opacity 0-1 (default: 1.0)
 *   - duration (number): Effect duration in seconds (default: 2.0)
 *   - loop (bool): Loop the effect (default: false)
 *
 * Outputs:
 *   - vfx_id (string): ID of spawned effect
 */
class WorkflowVfxSpawnStep : public IWorkflowStep {
public:
    explicit WorkflowVfxSpawnStep(std::shared_ptr<ILogger> logger);
    ~WorkflowVfxSpawnStep() override = default;

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    int nextVfxId_ = 0;
};

}  // namespace sdl3cpp::services::impl
