#include "services/interfaces/workflow/workflow_shader_builtin_constant_color_step.hpp"

#include "services/interfaces/i_graphics_service.hpp"
#include "services/interfaces/config_types.hpp"
#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowShaderBuiltinConstantColorStep::WorkflowShaderBuiltinConstantColorStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IGraphicsService> graphicsService)
    : logger_(std::move(logger)),
      graphicsService_(std::move(graphicsService)) {
    if (logger_) {
        logger_->Trace("WorkflowShaderBuiltinConstantColorStep", "Constructor", "Entry");
    }
}

std::string WorkflowShaderBuiltinConstantColorStep::GetPluginId() const {
    return "shader.builtin.constant_color";
}

void WorkflowShaderBuiltinConstantColorStep::Execute(const WorkflowStepDefinition& step,
                                                     WorkflowContext& context) {
    (void)step;  // Unused

    if (logger_) {
        logger_->Trace("WorkflowShaderBuiltinConstantColorStep", "Execute", "Entry",
                      "Generating built-in constant color shader");
    }

    if (!graphicsService_) {
        if (logger_) {
            logger_->Error("WorkflowShaderBuiltinConstantColorStep::Execute: No graphics service available");
        }
        context.Set<std::string>("shader.builtin_status", "failed");
        context.Set<std::string>("shader.error_message", "Graphics service not available");
        return;
    }

    // TODO: Implement proper constant color shader generation for SDL3 GPU pipeline
    // SDL3 GPU accepts SPIR-V or MSL shaders directly

    if (logger_) {
        logger_->Warn("WorkflowShaderBuiltinConstantColorStep: Built-in constant color shader not yet implemented");
    }

    context.Set<std::string>("shader.builtin_status", "not_implemented");

    if (logger_) {
        logger_->Trace("WorkflowShaderBuiltinConstantColorStep", "Execute", "Exit");
    }
}

}  // namespace sdl3cpp::services::impl
