#include "services/interfaces/workflow/workflow_generic_steps/workflow_model_despawn_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/scene_types.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {
std::string ReadObjectType(const WorkflowStepDefinition& step,
                           const WorkflowContext& context,
                           const WorkflowStepParameterResolver& parameterResolver) {
    auto it = step.inputs.find("object_type");
    if (it != step.inputs.end()) {
        const auto* value = context.TryGet<std::string>(it->second);
        if (!value) {
            throw std::runtime_error("model.despawn requires object_type string input");
        }
        return *value;
    }
    if (const auto* param = parameterResolver.FindParameter(step, "object_type")) {
        if (param->type != WorkflowParameterValue::Type::String) {
            throw std::runtime_error("model.despawn parameter 'object_type' must be string");
        }
        return param->stringValue;
    }
    throw std::runtime_error("model.despawn requires object_type");
}
}  // namespace

WorkflowModelDespawnStep::WorkflowModelDespawnStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowModelDespawnStep::GetPluginId() const {
    return "model.despawn";
}

void WorkflowModelDespawnStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    WorkflowStepParameterResolver parameterResolver;
    const std::string objectsKey = resolver.GetRequiredInputKey(step, "objects");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "objects");

    const auto* objects = context.TryGet<std::vector<SceneObject>>(objectsKey);
    if (!objects) {
        throw std::runtime_error("model.despawn requires objects list input");
    }

    const std::string objectType = ReadObjectType(step, context, parameterResolver);

    std::vector<SceneObject> remaining;
    remaining.reserve(objects->size());
    for (const auto& object : *objects) {
        if (object.objectType != objectType) {
            remaining.push_back(object);
        }
    }

    context.Set(outputKey, std::move(remaining));

    if (logger_) {
        logger_->Trace("WorkflowModelDespawnStep", "Execute",
                       "object_type=" + objectType +
                           ", output=" + outputKey,
                       "Removed model(s) from workflow list");
    }
}

}  // namespace sdl3cpp::services::impl
