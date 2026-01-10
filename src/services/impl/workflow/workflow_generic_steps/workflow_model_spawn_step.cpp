#include "workflow_model_spawn_step.hpp"

#include "../workflow_mesh_payload_converter.hpp"
#include "../workflow_step_io_resolver.hpp"
#include "../workflow_step_parameter_resolver.hpp"
#include "../../interfaces/scene_types.hpp"

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
            throw std::runtime_error("model.spawn requires matrix list of 16 numbers");
        }
        std::array<float, 16> matrix{};
        for (size_t i = 0; i < 16; ++i) {
            matrix[i] = static_cast<float>((*list)[i]);
        }
        return matrix;
    }
    if (const auto* param = parameterResolver.FindParameter(step, "matrix")) {
        if (param->type != WorkflowParameterValue::Type::NumberList || param->numberList.size() != 16u) {
            throw std::runtime_error("model.spawn parameter 'matrix' must be number list of 16");
        }
        std::array<float, 16> matrix{};
        for (size_t i = 0; i < 16; ++i) {
            matrix[i] = static_cast<float>(param->numberList[i]);
        }
        return matrix;
    }
    return {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

std::vector<std::string> ReadShaderKeys(const WorkflowStepDefinition& step,
                                        const WorkflowContext& context,
                                        const WorkflowStepParameterResolver& parameterResolver) {
    auto it = step.inputs.find("shader_keys");
    if (it != step.inputs.end()) {
        const auto* list = context.TryGet<std::vector<std::string>>(it->second);
        if (!list) {
            throw std::runtime_error("model.spawn requires shader_keys list input");
        }
        return *list;
    }
    auto singleIt = step.inputs.find("shader_key");
    if (singleIt != step.inputs.end()) {
        const auto* value = context.TryGet<std::string>(singleIt->second);
        if (!value) {
            throw std::runtime_error("model.spawn requires shader_key string input");
        }
        return { *value };
    }
    if (const auto* param = parameterResolver.FindParameter(step, "shader_keys")) {
        if (param->type != WorkflowParameterValue::Type::StringList) {
            throw std::runtime_error("model.spawn parameter 'shader_keys' must be string list");
        }
        return param->stringList;
    }
    if (const auto* param = parameterResolver.FindParameter(step, "shader_key")) {
        if (param->type != WorkflowParameterValue::Type::String) {
            throw std::runtime_error("model.spawn parameter 'shader_key' must be string");
        }
        return { param->stringValue };
    }
    throw std::runtime_error("model.spawn requires shader_key(s)");
}

std::string ReadObjectType(const WorkflowStepDefinition& step,
                           const WorkflowContext& context,
                           const WorkflowStepParameterResolver& parameterResolver) {
    auto it = step.inputs.find("object_type");
    if (it != step.inputs.end()) {
        const auto* value = context.TryGet<std::string>(it->second);
        if (!value) {
            throw std::runtime_error("model.spawn requires object_type string input");
        }
        return *value;
    }
    if (const auto* param = parameterResolver.FindParameter(step, "object_type")) {
        if (param->type != WorkflowParameterValue::Type::String) {
            throw std::runtime_error("model.spawn parameter 'object_type' must be string");
        }
        return param->stringValue;
    }
    return {};
}
}  // namespace

WorkflowModelSpawnStep::WorkflowModelSpawnStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowModelSpawnStep::GetPluginId() const {
    return "model.spawn";
}

void WorkflowModelSpawnStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    WorkflowStepParameterResolver parameterResolver;
    const std::string meshKey = resolver.GetRequiredInputKey(step, "mesh");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "objects");

    const auto* mesh = context.TryGet<MeshPayload>(meshKey);
    if (!mesh) {
        throw std::runtime_error("model.spawn requires mesh input");
    }

    std::vector<SceneObject> objects;
    auto objectsIt = step.inputs.find("objects");
    if (objectsIt != step.inputs.end()) {
        const auto* existing = context.TryGet<std::vector<SceneObject>>(objectsIt->second);
        if (!existing) {
            throw std::runtime_error("model.spawn requires objects list input");
        }
        objects = *existing;
    }

    MeshPayloadConversionResult conversion = ConvertMeshPayload(*mesh);
    SceneObject object;
    object.vertices = std::move(conversion.vertices);
    object.indices = std::move(conversion.indices);
    object.shaderKeys = ReadShaderKeys(step, context, parameterResolver);
    object.objectType = ReadObjectType(step, context, parameterResolver);
    object.computeModelMatrixRef = -1;
    object.modelMatrix = ReadMatrix(step, context, parameterResolver);
    object.hasCustomModelMatrix = true;
    objects.push_back(std::move(object));

    context.Set(outputKey, std::move(objects));

    if (logger_) {
        logger_->Trace("WorkflowModelSpawnStep", "Execute",
                       "mesh=" + meshKey +
                           ", output=" + outputKey,
                       "Spawned model into workflow list");
    }
}

}  // namespace sdl3cpp::services::impl
