#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Workflow step to update particle system physics and aging
 *
 * Plugin ID: particle.update
 *
 * Parameters:
 *   - delta_time (number): Time delta in seconds (default: frame.elapsed if available)
 *   - gravity (number): Gravity acceleration (default: 9.81)
 *   - damping (number): Velocity damping factor 0-1 (default: 1.0, no damping)
 *   - enable_fade (bool): Enable fade-out near end of lifetime (default: false)
 *
 * Inputs:
 *   - delta_time (number): Optional frame delta time
 *
 * Outputs:
 *   - particle_count (number): Number of active particles remaining
 */
class WorkflowParticleUpdateStep : public IWorkflowStep {
public:
    explicit WorkflowParticleUpdateStep(std::shared_ptr<ILogger> logger);
    ~WorkflowParticleUpdateStep() override = default;

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
