#include "services/interfaces/workflow/shader_compile_step_helpers.hpp"
#include "services/interfaces/workflow/shader_compile_build_map.hpp"
#include "services/interfaces/workflow/shader_compile_debug_marker.hpp"
#include "services/interfaces/workflow/shader_compile_gpu_load.hpp"

#include <exception>
#include <vector>

namespace sdl3cpp::services::impl {

void CompileShadersToContext(
    const std::shared_ptr<IShaderSystemRegistry>& shaderRegistry,
    const std::shared_ptr<IGraphicsService>& graphicsService,
    const std::shared_ptr<ILogger>& logger, WorkflowContext& context) {
    try {
        const auto shaderMap =
            BuildShaderMapWithMarkers(shaderRegistry, logger);

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
        LoadShaderMapToGpu(graphicsService, shaderMap, logger);

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
        context.Set<std::string>("shader.error_message", std::string(e.what()));
    }
}

}  // namespace sdl3cpp::services::impl
