#include "services/interfaces/workflow/shader_compile_step_helpers.hpp"

#include <fstream>
#include <vector>

namespace sdl3cpp::services::impl {

void WriteShaderCompileDebugMarker(const std::string& path,
                                   const std::string& content) {
    try {
        std::ofstream f(path);
        f << content;
        f.close();
    } catch (...) {
    }
}

void CompileShadersToContext(
    const std::shared_ptr<IShaderSystemRegistry>& shaderRegistry,
    const std::shared_ptr<IGraphicsService>& graphicsService,
    const std::shared_ptr<ILogger>& logger, WorkflowContext& context) {
    try {
        if (logger) {
            logger->Info(
                "WorkflowShaderCompileStep::Execute: Building shader map "
                "from active system");
        }

        WriteShaderCompileDebugMarker(
            "test_outputs/about_to_build_shader_map.txt",
            std::string("About to call shaderRegistry_->BuildShaderMap()\n") +
                "  shaderRegistry_: " +
                (shaderRegistry ? "VALID" : "NULL") + "\n");

        // Build shader map using active shader system.
        const auto shaderMap = shaderRegistry->BuildShaderMap();

        WriteShaderCompileDebugMarker(
            "test_outputs/after_build_shader_map.txt",
            "After shaderRegistry_->BuildShaderMap()\n"
            "  shaderMap.size(): " +
                std::to_string(shaderMap.size()) + "\n");

        if (logger) {
            logger->Info(
                "WorkflowShaderCompileStep::Execute: Shader compilation "
                "generated " +
                std::to_string(shaderMap.size()) + " shader(s)");
        }

        // Extract shader keys and convert to vector.
        std::vector<std::string> shaderKeys;
        for (const auto& pair : shaderMap) {
            shaderKeys.push_back(pair.first);
            if (logger) {
                logger->Trace("WorkflowShaderCompileStep", "Execute",
                             "shaderKey=" + pair.first);
            }
        }

        // Load compiled shaders to GPU if graphics service available.
        if (graphicsService) {
            try {
                if (logger) {
                    logger->Info(
                        "WorkflowShaderCompileStep::Execute: Loading "
                        "compiled shaders to GPU");
                }
                graphicsService->LoadShaders(shaderMap);
                if (logger) {
                    logger->Info(
                        "WorkflowShaderCompileStep::Execute: Shaders "
                        "loaded to GPU successfully");
                }
            } catch (const std::exception& e) {
                if (logger) {
                    logger->Warn(
                        "WorkflowShaderCompileStep::Execute: Graphics "
                        "service shader loading failed: " +
                        std::string(e.what()));
                }
                // Don't fail entirely - shaders are compiled even if GPU
                // load fails.
            }
        }

        context.Set<int>("shader.compiled_count",
                         static_cast<int>(shaderKeys.size()));
        context.Set<std::vector<std::string>>("shader.keys", shaderKeys);
        context.Set<std::string>("shader.compile_status", "success");

        if (logger) {
            logger->Trace("WorkflowShaderCompileStep", "Execute",
                         "Status: shader compilation successful, " +
                             std::to_string(shaderKeys.size()) +
                             " shaders available");
        }
    } catch (const std::exception& e) {
        WriteShaderCompileDebugMarker(
            "test_outputs/shader_compile_exception.txt",
            std::string("Exception in shader.compile:\n") + "  " + e.what() +
                "\n");

        if (logger) {
            logger->Error(
                "WorkflowShaderCompileStep::Execute: Shader compilation "
                "failed: " +
                std::string(e.what()));
        }

        context.Set<int>("shader.compiled_count", 0);
        context.Set<std::vector<std::string>>("shader.keys",
                                              std::vector<std::string>());
        context.Set<std::string>("shader.compile_status", "failed");
        context.Set<std::string>("shader.error_message",
                                 std::string(e.what()));
    }
}

}  // namespace sdl3cpp::services::impl
