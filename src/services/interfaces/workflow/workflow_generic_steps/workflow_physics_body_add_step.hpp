#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: physics.body.add
 *
 * Creates a box or capsule rigid body and adds it to the physics world,
 * storing the body/shape pointers and visual metadata in context under
 * "physics_body_<name>"/"physics_shape_<name>"/"physics_visual_<name>",
 * and appending <name> to the "physics_bodies" registry.
 *
 * See physics_body_builder.hpp for the shape/body construction.
 */
class WorkflowPhysicsBodyAddStep final : public IWorkflowStep {
public:
    explicit WorkflowPhysicsBodyAddStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
