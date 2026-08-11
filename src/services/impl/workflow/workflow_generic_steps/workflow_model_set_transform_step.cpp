#include "services/interfaces/workflow/workflow_generic_steps/workflow_model_set_transform_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/scene_types.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {
std::array<float, 16> ReadMatrix(const WorkflowStepDefinition& step,
                                 const WorkflowContext& context,
                                 const WorkflowStepParameterResolver& parameterResolver) {
    auto it = step.inputs.find("matrix");
    if (it != step.inputs.end()) {
        const auto* list = context.TryGet<std::vector<double>>(it->second);
        if (!list || list->size() != 16u) {
            throw std::runtime_error("model.set_transform requires matrix list of 16 numbers");
        }
        std::array<float, 16> matrix{};
        for (size_t i = 0; i < 16; ++i) {
            matrix[i] = static_cast<float>((*list)[i]);
        }
        return matrix;
    }
    if (const auto* param = parameterResolver.FindParameter(step, "matrix")) {
        if (param->type != WorkflowParameterValue::Type::NumberList || param->numberList.size() != 16u) {
            throw std::runtime_error("model.set_transform parameter 'matrix' must be number list of 16");
        }
        std::array<float, 16> matrix{};
        for (size_t i = 0; i < 16; ++i) {
            matrix[i] = static_cast<float>(param->numberList[i]);
        }
        return matrix;
    }
    throw std::runtime_error("model.set_transform requires matrix input");
}

std::string ReadObjectType(const WorkflowStepDefinition& step,
                           const WorkflowContext& context,
                           const WorkflowStepParameterResolver& parameterResolver) {
    auto it = step.inputs.find("object_type");
    if (it != step.inputs.end()) {
        const auto* value = context.TryGet<std::string>(it->second);
        if (!value) {
            throw std::runtime_error("model.set_transform requires object_type string input");
        }
        return *value;
    }
    if (const auto* param = parameterResolver.FindParameter(step, "object_type")) {
        if (param->type != WorkflowParameterValue::Type::String) {
            throw std::runtime_error("model.set_transform parameter 'object_type' must be string");
        }
        return param->stringValue;
    }
    throw std::runtime_error("model.set_transform requires object_type");
}
}  // namespace

WorkflowModelSetTransformStep::WorkflowModelSetTransformStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowModelSetTransformStep::GetPluginId() const {
    return "model.set_transform";
}

void WorkflowModelSetTransformStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    WorkflowStepParameterResolver parameterResolver;
    const std::string objectsKey = resolver.GetRequiredInputKey(step, "objects");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "objects");

    const auto* objects = context.TryGet<std::vector<SceneObject>>(objectsKey);
    if (!objects) {
        throw std::runtime_error("model.set_transform requires objects list input");
    }

    const std::string objectType = ReadObjectType(step, context, parameterResolver);
    const std::array<float, 16> matrix = ReadMatrix(step, context, parameterResolver);

    std::vector<SceneObject> updated = *objects;
    for (auto& object : updated) {
        if (object.objectType == objectType) {
            object.modelMatrix = matrix;
            object.hasCustomModelMatrix = true;
            object.computeModelMatrixRef = -1;
        }
    }

    context.Set(outputKey, std::move(updated));

    if (logger_) {
        logger_->Trace("WorkflowModelSetTransformStep", "Execute",
                       "object_type=" + objectType +
                           ", output=" + outputKey,
                       "Updated model transform");
    }
}

}  // namespace sdl3cpp::services::impl
