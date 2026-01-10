#include "workflow_frame_bullet_physics_step.hpp"
#include "../workflow_step_io_resolver.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowFrameBulletPhysicsStep::WorkflowFrameBulletPhysicsStep(std::shared_ptr<IPhysicsService> physicsService,
                                                               std::shared_ptr<ILogger> logger)
    : physicsService_(std::move(physicsService)),
      logger_(std::move(logger)) {}

std::string WorkflowFrameBulletPhysicsStep::GetPluginId() const {
    return "frame.bullet_physics";
}

void WorkflowFrameBulletPhysicsStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!physicsService_) {
        throw std::runtime_error("frame.bullet_physics requires an IPhysicsService");
    }
    WorkflowStepIoResolver resolver;
    const std::string deltaKey = resolver.GetRequiredInputKey(step, "delta");
    const auto* delta = context.TryGet<double>(deltaKey);
    if (!delta) {
        throw std::runtime_error("frame.bullet_physics missing delta input");
    }
    physicsService_->StepSimulation(static_cast<float>(*delta));
    if (logger_) {
        logger_->Trace("WorkflowFrameBulletPhysicsStep", "Execute",
                       "delta=" + std::to_string(*delta),
                       "Bullet physics step completed");
    }
}

}  // namespace sdl3cpp::services::impl
