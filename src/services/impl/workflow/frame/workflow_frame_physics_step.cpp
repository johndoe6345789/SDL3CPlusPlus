#include "workflow_frame_physics_step.hpp"
#include "../workflow_step_io_resolver.hpp"

#include "services/interfaces/i_physics_service.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowFramePhysicsStep::WorkflowFramePhysicsStep(std::shared_ptr<IPhysicsService> physicsService,
                                                   std::shared_ptr<ILogger> logger)
    : physicsService_(std::move(physicsService)),
      logger_(std::move(logger)) {}

std::string WorkflowFramePhysicsStep::GetPluginId() const {
    return "frame.physics";
}

void WorkflowFramePhysicsStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!physicsService_) {
        throw std::runtime_error("frame.physics requires an IPhysicsService");
    }
    WorkflowStepIoResolver resolver;
    const std::string deltaKey = resolver.GetRequiredInputKey(step, "delta");
    const auto* delta = context.TryGet<double>(deltaKey);
    if (!delta) {
        throw std::runtime_error("frame.physics missing delta input");
    }
    physicsService_->StepSimulation(static_cast<float>(*delta));
    if (logger_) {
        logger_->Trace("WorkflowFramePhysicsStep", "Execute",
                       "delta=" + std::to_string(*delta),
                       "Physics step completed");
    }
}

}  // namespace sdl3cpp::services::impl
