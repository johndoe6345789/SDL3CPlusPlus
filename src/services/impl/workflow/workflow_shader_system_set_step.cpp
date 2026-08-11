#include "services/interfaces/workflow/workflow_shader_system_set_step.hpp"

#include "services/interfaces/i_shader_system_registry.hpp"
#include "services/interfaces/workflow_definition.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowShaderSystemSetStep::WorkflowShaderSystemSetStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IShaderSystemRegistry> shaderRegistry)
    : logger_(std::move(logger)),
      shaderRegistry_(std::move(shaderRegistry)) {
}

std::string WorkflowShaderSystemSetStep::GetPluginId() const {
    return "shader.system.set";
}

void WorkflowShaderSystemSetStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowShaderSystemSetStep", "Execute", "Entry",
                      "Setting active shader system");
    }

    WorkflowStepParameterResolver paramResolver;

    // Get the shader system ID from parameters
    std::string systemId = "glsl";  // Default to GLSL (MaterialX removed)
    if (const auto* param = paramResolver.FindParameter(step, "system_id")) {
        if (param->type == WorkflowParameterValue::Type::String) {
            systemId = param->stringValue;
        }
    }

    if (logger_) {
        logger_->Trace("WorkflowShaderSystemSetStep", "Execute",
                      "Parameters: system_id=" + systemId,
                      "Configuring shader system");
    }

    try {
        if (!shaderRegistry_) {
            throw std::runtime_error("Shader registry unavailable");
        }

        // Store system selection in context for workflow coordination
        context.Set("shader.system.selected_id", systemId);
        context.Set("shader.system.selection_status", "set");

        if (logger_) {
            logger_->Trace("WorkflowShaderSystemSetStep", "Execute",
                          "Shader system set to: " + systemId,
                          "Ready for shader operations");
        }

        // Set success status
        context.Set("shader.system.error_message", "");

    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("WorkflowShaderSystemSetStep::Execute: " + std::string(e.what()));
        }
        context.Set("shader.system.selection_status", "error");
        context.Set("shader.system.error_message", e.what());
        throw;
    }
}

}  // namespace sdl3cpp::services::impl
