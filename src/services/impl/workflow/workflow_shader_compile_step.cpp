#include "services/interfaces/workflow/workflow_shader_compile_step.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

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

    // Debug marker
    try {
        std::ofstream f("test_outputs/shader_compile_step_executed.txt");
        f << "WorkflowShaderCompileStep::Execute() was called\n";
        f.close();
    } catch (...) {}

    if (logger_) {
        logger_->Trace("WorkflowShaderCompileStep", "Execute", "Entry");
    }

    if (!shaderRegistry_) {
        if (logger_) {
            logger_->Error("WorkflowShaderCompileStep::Execute: No shader registry available");
        }
        context.Set<std::string>("shader.compile_status", "failed");
        context.Set<std::string>("shader.error_message", "Shader registry not available");
        return;
    }

    try {
        if (logger_) {
            logger_->Info("WorkflowShaderCompileStep::Execute: Building shader map from active system");
        }

        // Debug: about to call BuildShaderMap
        std::ofstream beforeFile("test_outputs/about_to_build_shader_map.txt");
        beforeFile << "About to call shaderRegistry_->BuildShaderMap()\n";
        beforeFile << "  shaderRegistry_: " << (shaderRegistry_ ? "VALID" : "NULL") << "\n";
        beforeFile.close();

        // Build shader map using active shader system
        const auto shaderMap = shaderRegistry_->BuildShaderMap();

        // Debug: after call
        std::ofstream afterFile("test_outputs/after_build_shader_map.txt");
        afterFile << "After shaderRegistry_->BuildShaderMap()\n";
        afterFile << "  shaderMap.size(): " << shaderMap.size() << "\n";
        afterFile.close();


        if (logger_) {
            logger_->Info("WorkflowShaderCompileStep::Execute: Shader compilation generated " +
                         std::to_string(shaderMap.size()) + " shader(s)");
        }

        // Extract shader keys and convert to vector
        std::vector<std::string> shaderKeys;
        for (const auto& pair : shaderMap) {
            shaderKeys.push_back(pair.first);
            if (logger_) {
                logger_->Trace("WorkflowShaderCompileStep", "Execute",
                              "shaderKey=" + pair.first);
            }
        }

        // Load compiled shaders to GPU if graphics service available
        if (graphicsService_) {
            try {
                if (logger_) {
                    logger_->Info("WorkflowShaderCompileStep::Execute: Loading compiled shaders to GPU");
                }
                graphicsService_->LoadShaders(shaderMap);
                if (logger_) {
                    logger_->Info("WorkflowShaderCompileStep::Execute: Shaders loaded to GPU successfully");
                }
            } catch (const std::exception& e) {
                if (logger_) {
                    logger_->Warn("WorkflowShaderCompileStep::Execute: Graphics service shader loading failed: " +
                                 std::string(e.what()));
                }
                // Don't fail entirely - shaders are compiled even if GPU load fails
            }
        }

        // Store results in context
        context.Set<int>("shader.compiled_count", static_cast<int>(shaderKeys.size()));
        context.Set<std::vector<std::string>>("shader.keys", shaderKeys);
        context.Set<std::string>("shader.compile_status", "success");

        if (logger_) {
            logger_->Trace("WorkflowShaderCompileStep", "Execute",
                          "Status: shader compilation successful, " +
                          std::to_string(shaderKeys.size()) + " shaders available");
        }

    } catch (const std::exception& e) {
        // Debug: exception occurred
        try {
            std::ofstream f("test_outputs/shader_compile_exception.txt");
            f << "Exception in shader.compile:\n";
            f << "  " << e.what() << "\n";
            f.close();
        } catch (...) {}

        if (logger_) {
            logger_->Error("WorkflowShaderCompileStep::Execute: Shader compilation failed: " +
                          std::string(e.what()));
        }

        context.Set<int>("shader.compiled_count", 0);
        context.Set<std::vector<std::string>>("shader.keys", std::vector<std::string>());
        context.Set<std::string>("shader.compile_status", "failed");
        context.Set<std::string>("shader.error_message", std::string(e.what()));
    }

    if (logger_) {
        logger_->Trace("WorkflowShaderCompileStep", "Execute", "Exit");
    }
}

}  // namespace sdl3cpp::services::impl
