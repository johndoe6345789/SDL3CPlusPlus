#include "services/interfaces/workflow/workflow_generic_steps/workflow_model_set_transform_step.hpp"

#include "services/interfaces/workflow/workflow_generic_steps/model_input_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/scene_types.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowModelSetTransformStep::WorkflowModelSetTransformStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowModelSetTransformStep::GetPluginId() const {
    return "model.set_transform";
}

void WorkflowModelSetTransformStep::Execute(const WorkflowStepDefinition& step,
                                            WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    WorkflowStepParameterResolver parameterResolver;
    const std::string objectsKey =
        resolver.GetRequiredInputKey(step, "objects");
    const std::string outputKey =
        resolver.GetRequiredOutputKey(step, "objects");

    const auto* objects = context.TryGet<std::vector<SceneObject>>(objectsKey);
    if (!objects) {
        throw std::runtime_error(
            "model.set_transform requires objects list input");
    }

    const std::string objectType = ReadObjectTypeInput(
        step, context, parameterResolver, "model.set_transform", nullptr);
    const std::array<float, 16> matrix = ReadMatrixInput(
        step, context, parameterResolver, "model.set_transform", nullptr);

    std::vector<SceneObject> updated = *objects;
    for (auto& object : updated) {
        if (object.objectType == objectType) {
            object.modelMatrix           = matrix;
            object.hasCustomModelMatrix  = true;
            object.computeModelMatrixRef = -1;
        }
    }

    context.Set(outputKey, std::move(updated));

    if (logger_) {
        logger_->Trace("WorkflowModelSetTransformStep", "Execute",
                       "object_type=" + objectType + ", output=" + outputKey,
                       "Updated model transform");
    }
}

}  // namespace sdl3cpp::services::impl
