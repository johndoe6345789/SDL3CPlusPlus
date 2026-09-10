#include "services/interfaces/workflow/workflow_shader_system_initialize_step.hpp"

#include "services/interfaces/workflow_definition.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {

WorkflowShaderSystemInitializeStep::WorkflowShaderSystemInitializeStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowShaderSystemInitializeStep::GetPluginId() const {
    return "shader.system.initialize";
}

void WorkflowShaderSystemInitializeStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowShaderSystemInitializeStep", "Execute",
                      "Phase 1: Setting shader system",
                      "Configuring active shader system");
    }

    WorkflowStepParameterResolver paramResolver;

    // Default to GLSL; glTF is asset loader, not shader system.
    std::string systemId = "glsl";
    if (const auto* param = paramResolver.FindParameter(step, "system_id")) {
        if (param->type == WorkflowParameterValue::Type::String) {
            systemId = param->stringValue;
        }
    }

    context.Set("shader.system.selected_id", systemId);
    context.Set("shader.system.selection_status", "set");

    if (logger_) {
        logger_->Trace("WorkflowShaderSystemInitializeStep", "Execute",
                      "Shader system set to: " + systemId,
                      "Phase 1 complete");
    }
}

}  // namespace sdl3cpp::services::impl
