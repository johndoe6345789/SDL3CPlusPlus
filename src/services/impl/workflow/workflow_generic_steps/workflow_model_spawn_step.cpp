#include "services/interfaces/workflow/workflow_generic_steps/workflow_model_spawn_step.hpp"

#include "services/interfaces/workflow/workflow_generic_steps/model_spawn_input_helpers.hpp"
#include "services/interfaces/workflow/workflow_mesh_payload_converter.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/scene_types.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowModelSpawnStep::WorkflowModelSpawnStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowModelSpawnStep::GetPluginId() const {
    return "model.spawn";
}

void WorkflowModelSpawnStep::Execute(const WorkflowStepDefinition& step,
                                     WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    WorkflowStepParameterResolver parameterResolver;
    const std::string meshKey = resolver.GetRequiredInputKey(step, "mesh");
    const std::string outputKey =
        resolver.GetRequiredOutputKey(step, "objects");

    const auto* mesh = context.TryGet<MeshPayload>(meshKey);
    if (!mesh) {
        throw std::runtime_error("model.spawn requires mesh input");
    }

    std::vector<SceneObject> objects;
    auto objectsIt = step.inputs.find("objects");
    if (objectsIt != step.inputs.end()) {
        const auto* existing =
            context.TryGet<std::vector<SceneObject>>(objectsIt->second);
        if (!existing) {
            throw std::runtime_error(
                "model.spawn requires objects list input");
        }
        objects = *existing;
    }

    MeshPayloadConversionResult conversion = ConvertMeshPayload(*mesh);
    SceneObject object;
    object.vertices     = std::move(conversion.vertices);
    object.indices       = std::move(conversion.indices);
    object.shaderKeys    = ReadShaderKeys(step, context, parameterResolver);
    object.objectType    = ReadObjectType(step, context, parameterResolver);
    object.computeModelMatrixRef = -1;
    object.modelMatrix    = ReadMatrix(step, context, parameterResolver);
    object.hasCustomModelMatrix = true;
    objects.push_back(std::move(object));

    context.Set(outputKey, std::move(objects));

    if (logger_) {
        logger_->Trace("WorkflowModelSpawnStep", "Execute",
                       "mesh=" + meshKey + ", output=" + outputKey,
                       "Spawned model into workflow list");
    }
}

}  // namespace sdl3cpp::services::impl
