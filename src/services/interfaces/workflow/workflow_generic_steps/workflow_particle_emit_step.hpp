#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Workflow step to emit particles from a location
 *
 * Plugin ID: particle.emit
 *
 * Parameters:
 *   - emitter_id (string): Unique identifier for the emitter
 *   - count (number): Number of particles to emit (default: 10)
 *   - position_x/y/z (number): Initial position (default: 0,0,0)
 *   - velocity_x/y/z (number): Initial velocity (default: 0,0,0)
 *   - velocity_spread (number): Velocity variance (default: 0.0)
 *   - speed (number): Initial speed magnitude (default: 1.0)
 *   - lifetime (number): Particle lifespan in seconds (default: 2.0)
 *   - lifetime_variance (number): Lifetime variation (default: 0.0)
 *   - color_r/g/b (number): RGB color 0-255 (default: 255,255,255)
 *   - size (number): Particle size (default: 1.0)
 *   - size_variance (number): Size variation (default: 0.0)
 *   - random_direction (bool): Use random direction (default: false)
 *   - spread_angle (number): Cone spread angle in degrees (default: 0)
 *
 * Outputs:
 *   - particle_ids (list): List of spawned particle IDs
 */
class WorkflowParticleEmitStep : public IWorkflowStep {
public:
    explicit WorkflowParticleEmitStep(std::shared_ptr<ILogger> logger);
    ~WorkflowParticleEmitStep() override = default;

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
