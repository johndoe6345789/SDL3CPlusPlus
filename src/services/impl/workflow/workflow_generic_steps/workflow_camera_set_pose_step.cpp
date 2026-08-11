#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_set_pose_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {
std::array<float, 3> ReadVec3(const WorkflowStepDefinition& step,
                              const WorkflowContext& context,
                              const WorkflowStepParameterResolver& parameterResolver,
                              const std::string& name,
                              const std::array<float, 3>& fallback) {
    auto it = step.inputs.find(name);
    if (it != step.inputs.end()) {
        const auto* list = context.TryGet<std::vector<double>>(it->second);
        if (!list || list->size() != 3u) {
            throw std::runtime_error("camera.set_pose requires '" + name + "' list of 3 numbers");
        }
        return {static_cast<float>((*list)[0]),
                static_cast<float>((*list)[1]),
                static_cast<float>((*list)[2])};
    }
    if (const auto* param = parameterResolver.FindParameter(step, name)) {
        if (param->type != WorkflowParameterValue::Type::NumberList || param->numberList.size() != 3u) {
            throw std::runtime_error("camera.set_pose parameter '" + name + "' must be number list of 3");
        }
        return {static_cast<float>(param->numberList[0]),
                static_cast<float>(param->numberList[1]),
                static_cast<float>(param->numberList[2])};
    }
    return fallback;
}

float ReadNumber(const WorkflowStepDefinition& step,
                 const WorkflowContext& context,
                 const WorkflowStepParameterResolver& parameterResolver,
                 const std::string& name,
                 float fallback) {
    auto it = step.inputs.find(name);
    if (it != step.inputs.end()) {
        const auto* value = context.TryGet<double>(it->second);
        if (!value) {
            throw std::runtime_error("camera.set_pose requires number input '" + name + "'");
        }
        return static_cast<float>(*value);
    }
    if (const auto* param = parameterResolver.FindParameter(step, name)) {
        if (param->type != WorkflowParameterValue::Type::Number) {
            throw std::runtime_error("camera.set_pose parameter '" + name + "' must be a number");
        }
        return static_cast<float>(param->numberValue);
    }
    return fallback;
}
}  // namespace

WorkflowCameraSetPoseStep::WorkflowCameraSetPoseStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowCameraSetPoseStep::GetPluginId() const {
    return "camera.set_pose";
}

void WorkflowCameraSetPoseStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver ioResolver;
    WorkflowStepParameterResolver parameterResolver;
    const std::string outputKey = ioResolver.GetRequiredOutputKey(step, "pose");

    CameraPose pose;
    pose.position = ReadVec3(step, context, parameterResolver, "position", pose.position);
    pose.lookAt = ReadVec3(step, context, parameterResolver, "look_at", pose.lookAt);
    pose.up = ReadVec3(step, context, parameterResolver, "up", pose.up);
    pose.fovDegrees = ReadNumber(step, context, parameterResolver, "fov_degrees", pose.fovDegrees);
    pose.nearPlane = ReadNumber(step, context, parameterResolver, "near", pose.nearPlane);
    pose.farPlane = ReadNumber(step, context, parameterResolver, "far", pose.farPlane);

    context.Set(outputKey, pose);

    if (logger_) {
        logger_->Trace("WorkflowCameraSetPoseStep", "Execute",
                       "output=" + outputKey,
                       "Set camera pose");
    }
}

}  // namespace sdl3cpp::services::impl
