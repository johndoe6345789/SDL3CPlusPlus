#include "services/interfaces/workflow/workflow_shader_system_compile_step.hpp"

#include "services/interfaces/i_shader_system_registry.hpp"
#include "services/interfaces/i_graphics_service.hpp"
#include "services/interfaces/i_workflow_executor.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowShaderSystemCompileStep::WorkflowShaderSystemCompileStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IShaderSystemRegistry> shaderRegistry,
    std::shared_ptr<IGraphicsService> graphicsService,
    std::shared_ptr<IWorkflowExecutor> workflowExecutor)
    : logger_(std::move(logger)),
      shaderRegistry_(std::move(shaderRegistry)),
      graphicsService_(std::move(graphicsService)),
      workflowExecutor_(std::move(workflowExecutor)) {}

std::string WorkflowShaderSystemCompileStep::GetPluginId() const {
    return "shader.system.compile";
}

void WorkflowShaderSystemCompileStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    (void)step;  // Unused

    if (logger_) {
        logger_->Trace("WorkflowShaderSystemCompileStep", "Execute",
                       "Phase 3: Compiling shaders",
                       "Building shader programs");
    }

    try {
        if (!shaderRegistry_ || !graphicsService_) {
            throw std::runtime_error(
                "Missing shader registry or graphics service");
        }

        // Compile shaders through registry
        context.Set("shader.compile_status", "compiling");

        // Build shader map and load to GPU
        const auto shaderMap = shaderRegistry_->BuildShaderMap();
        graphicsService_->LoadShaders(shaderMap);

        if (logger_) {
            logger_->Trace("WorkflowShaderSystemCompileStep", "Execute",
                           "Shader compilation succeeded", "Phase 3 complete");
        }

        context.Set("shader.compile_status", "compiled");
        context.Set("shader.compiled_count",
                    static_cast<double>(shaderMap.size()));

        if (logger_) {
            logger_->Info(
                "WorkflowShaderSystemCompileStep::Execute: "
                "Shader system initialization complete");
        }
        context.Set("shader.init_status", "complete");

    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("WorkflowShaderSystemCompileStep::Execute: " +
                           std::string(e.what()));
        }
        context.Set("shader.compile_status", "error");
        context.Set("shader.error_message", e.what());
        context.Set("shader.init_status", "error");
        throw;
    }
}

}  // namespace sdl3cpp::services::impl
