#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_teleport_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowCameraTeleportStep::WorkflowCameraTeleportStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowCameraTeleportStep::GetPluginId() const {
    return "camera.teleport";
}

void WorkflowCameraTeleportStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string poseKey = resolver.GetRequiredInputKey(step, "pose");
    const std::string positionKey = resolver.GetRequiredInputKey(step, "position");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "pose");

    const auto* pose = context.TryGet<CameraPose>(poseKey);
    const auto* position = context.TryGet<std::vector<double>>(positionKey);
    if (!pose || !position || position->size() != 3u) {
        throw std::runtime_error("camera.teleport requires pose and position[3] inputs");
    }

    CameraPose updated = *pose;
    updated.position = {static_cast<float>((*position)[0]),
                        static_cast<float>((*position)[1]),
                        static_cast<float>((*position)[2])};
    context.Set(outputKey, updated);

    if (logger_) {
        logger_->Trace("WorkflowCameraTeleportStep", "Execute",
                       "input=" + poseKey +
                           ", output=" + outputKey,
                       "Teleported camera");
    }
}

}  // namespace sdl3cpp::services::impl
