#include "services/interfaces/workflow/workflow_generic_steps/camera_set_pose_helpers.hpp"

#include <stdexcept>
#include <vector>

namespace sdl3cpp::services::impl {

std::array<float, 3> ReadCameraPoseVec3(
    const WorkflowStepDefinition& step, const WorkflowContext& context,
    const WorkflowStepParameterResolver& parameterResolver,
    const std::string& name, const std::array<float, 3>& fallback) {
    auto it = step.inputs.find(name);
    if (it != step.inputs.end()) {
        const auto* list = context.TryGet<std::vector<double>>(it->second);
        if (!list || list->size() != 3u) {
            throw std::runtime_error("camera.set_pose requires '" + name +
                                     "' list of 3 numbers");
        }
        return {static_cast<float>((*list)[0]), static_cast<float>((*list)[1]),
                static_cast<float>((*list)[2])};
    }
    if (const auto* param = parameterResolver.FindParameter(step, name)) {
        if (param->type != WorkflowParameterValue::Type::NumberList ||
            param->numberList.size() != 3u) {
            throw std::runtime_error("camera.set_pose parameter '" + name +
                                     "' must be number list of 3");
        }
        return {static_cast<float>(param->numberList[0]),
                static_cast<float>(param->numberList[1]),
                static_cast<float>(param->numberList[2])};
    }
    return fallback;
}

float ReadCameraPoseNumber(
    const WorkflowStepDefinition& step, const WorkflowContext& context,
    const WorkflowStepParameterResolver& parameterResolver,
    const std::string& name, float fallback) {
    auto it = step.inputs.find(name);
    if (it != step.inputs.end()) {
        const auto* value = context.TryGet<double>(it->second);
        if (!value) {
            throw std::runtime_error("camera.set_pose requires number input '" +
                                     name + "'");
        }
        return static_cast<float>(*value);
    }
    if (const auto* param = parameterResolver.FindParameter(step, name)) {
        if (param->type != WorkflowParameterValue::Type::Number) {
            throw std::runtime_error("camera.set_pose parameter '" + name +
                                     "' must be a number");
        }
        return static_cast<float>(param->numberValue);
    }
    return fallback;
}

}  // namespace sdl3cpp::services::impl
