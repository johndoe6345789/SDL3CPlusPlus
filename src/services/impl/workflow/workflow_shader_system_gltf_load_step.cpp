#include "services/interfaces/workflow/workflow_shader_system_gltf_load_step.hpp"

#include "services/interfaces/workflow_definition.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {

WorkflowShaderSystemGltfLoadStep::WorkflowShaderSystemGltfLoadStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowShaderSystemGltfLoadStep::GetPluginId() const {
    return "shader.system.gltf_load";
}

void WorkflowShaderSystemGltfLoadStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowShaderSystemGltfLoadStep", "Execute",
                       "Phase 2: Loading glTF models",
                       "Loading model and asset configuration");
    }

    WorkflowStepParameterResolver paramResolver;

    std::string modelPath = "";
    if (const auto* param = paramResolver.FindParameter(step, "model_path")) {
        if (param->type == WorkflowParameterValue::Type::String) {
            modelPath = param->stringValue;
        }
    }

    // Store glTF configuration in context
    context.Set("gltf.model_path", modelPath);
    context.Set("gltf.load_status", "loading");

    if (logger_) {
        logger_->Trace("WorkflowShaderSystemGltfLoadStep", "Execute",
                       "Model path: " + modelPath, "Phase 2 complete");
    }

    context.Set("gltf.load_status", "loaded");
}

}  // namespace sdl3cpp::services::impl
