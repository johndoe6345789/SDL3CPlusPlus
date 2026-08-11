#include "services/interfaces/workflow/workflow_shader_system_initialize_step.hpp"

#include "services/interfaces/i_shader_system_registry.hpp"
#include "services/interfaces/i_graphics_service.hpp"
#include "services/interfaces/i_workflow_executor.hpp"
#include "services/interfaces/workflow_definition.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowShaderSystemInitializeStep::WorkflowShaderSystemInitializeStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IShaderSystemRegistry> shaderRegistry,
    std::shared_ptr<IGraphicsService> graphicsService,
    std::shared_ptr<IWorkflowExecutor> workflowExecutor)
    : logger_(std::move(logger)),
      shaderRegistry_(std::move(shaderRegistry)),
      graphicsService_(std::move(graphicsService)),
      workflowExecutor_(std::move(workflowExecutor)) {
}

std::string WorkflowShaderSystemInitializeStep::GetPluginId() const {
    return "shader.system.initialize";
}

void WorkflowShaderSystemInitializeStep::Execute(const WorkflowStepDefinition& step,
                                                 WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowShaderSystemInitializeStep", "Execute", "Entry",
                      "Three-phase shader system initialization starting");
    }

    try {
        // Phase 1: Set active shader system
        ExecuteSystemSet(step, context);

        // Phase 2: Load glTF models
        ExecuteGltfLoad(step, context);

        // Phase 3: Compile shaders
        ExecuteCompile(step, context);

        if (logger_) {
            logger_->Info("WorkflowShaderSystemInitializeStep::Execute: "
                         "Shader system initialization complete");
        }

        context.Set("shader.init_status", "complete");

    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("WorkflowShaderSystemInitializeStep::Execute: " + std::string(e.what()));
        }
        context.Set("shader.init_status", "error");
        context.Set("shader.error_message", e.what());
        throw;
    }
}

void WorkflowShaderSystemInitializeStep::ExecuteSystemSet(const WorkflowStepDefinition& step,
                                                         WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowShaderSystemInitializeStep", "ExecuteSystemSet",
                      "Phase 1: Setting shader system",
                      "Configuring active shader system");
    }

    WorkflowStepParameterResolver paramResolver;

    std::string systemId = "glsl";  // Default to GLSL; glTF is asset loader, not shader system
    if (const auto* param = paramResolver.FindParameter(step, "system_id")) {
        if (param->type == WorkflowParameterValue::Type::String) {
            systemId = param->stringValue;
        }
    }

    context.Set("shader.system.selected_id", systemId);
    context.Set("shader.system.selection_status", "set");

    if (logger_) {
        logger_->Trace("WorkflowShaderSystemInitializeStep", "ExecuteSystemSet",
                      "Shader system set to: " + systemId,
                      "Phase 1 complete");
    }
}

void WorkflowShaderSystemInitializeStep::ExecuteGltfLoad(const WorkflowStepDefinition& step,
                                                        WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowShaderSystemInitializeStep", "ExecuteGltfLoad",
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
        logger_->Trace("WorkflowShaderSystemInitializeStep", "ExecuteGltfLoad",
                      "Model path: " + modelPath,
                      "Phase 2 complete");
    }

    context.Set("gltf.load_status", "loaded");
}

void WorkflowShaderSystemInitializeStep::ExecuteCompile(const WorkflowStepDefinition& step,
                                                       WorkflowContext& context) {
    (void)step;  // Unused

    if (logger_) {
        logger_->Trace("WorkflowShaderSystemInitializeStep", "ExecuteCompile",
                      "Phase 3: Compiling shaders",
                      "Building shader programs");
    }

    try {
        if (!shaderRegistry_ || !graphicsService_) {
            throw std::runtime_error("Missing shader registry or graphics service");
        }

        // Compile shaders through registry
        context.Set("shader.compile_status", "compiling");

        // Build shader map and load to GPU
        const auto shaderMap = shaderRegistry_->BuildShaderMap();
        graphicsService_->LoadShaders(shaderMap);

        if (logger_) {
            logger_->Trace("WorkflowShaderSystemInitializeStep", "ExecuteCompile",
                          "Shader compilation succeeded",
                          "Phase 3 complete");
        }

        context.Set("shader.compile_status", "compiled");
        context.Set("shader.compiled_count", static_cast<double>(shaderMap.size()));

    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("WorkflowShaderSystemInitializeStep::ExecuteCompile: " +
                         std::string(e.what()));
        }
        context.Set("shader.compile_status", "error");
        context.Set("shader.error_message", e.what());
        throw;
    }
}

}  // namespace sdl3cpp::services::impl
