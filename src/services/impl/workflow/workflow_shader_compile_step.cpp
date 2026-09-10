#include "services/interfaces/workflow/workflow_shader_compile_step.hpp"
#include "services/interfaces/workflow/shader_compile_step_helpers.hpp"

namespace sdl3cpp::services::impl {

WorkflowShaderCompileStep::WorkflowShaderCompileStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IShaderSystemRegistry> shaderRegistry,
    std::shared_ptr<IGraphicsService> graphicsService)
    : logger_(std::move(logger)),
      shaderRegistry_(std::move(shaderRegistry)),
      graphicsService_(std::move(graphicsService)) {
    if (logger_) {
        logger_->Trace("WorkflowShaderCompileStep", "Constructor", "Entry");
    }
}

std::string WorkflowShaderCompileStep::GetPluginId() const {
    return "shader.compile";
}

void WorkflowShaderCompileStep::Execute(const WorkflowStepDefinition& step,
                                        WorkflowContext& context) {
    (void)step;  // Unused

    WriteShaderCompileDebugMarker(
        "test_outputs/shader_compile_step_executed.txt",
        "WorkflowShaderCompileStep::Execute() was called\n");

    if (logger_) {
        logger_->Trace("WorkflowShaderCompileStep", "Execute", "Entry");
    }

    if (!shaderRegistry_) {
        if (logger_) {
            logger_->Error(
                "WorkflowShaderCompileStep::Execute: No shader registry "
                "available");
        }
        context.Set<std::string>("shader.compile_status", "failed");
        context.Set<std::string>("shader.error_message",
                                 "Shader registry not available");
        return;
    }

    CompileShadersToContext(shaderRegistry_, graphicsService_, logger_,
                           context);

    if (logger_) {
        logger_->Trace("WorkflowShaderCompileStep", "Execute", "Exit");
    }
}

}  // namespace sdl3cpp::services::impl
