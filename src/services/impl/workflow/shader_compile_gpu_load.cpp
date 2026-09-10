#include "services/interfaces/workflow/shader_compile_gpu_load.hpp"

#include <exception>
#include <string>

namespace sdl3cpp::services::impl {

void LoadShaderMapToGpu(
    const std::shared_ptr<IGraphicsService>& graphicsService,
    const std::unordered_map<std::string, ShaderPaths>& shaderMap,
    const std::shared_ptr<ILogger>& logger) {
    if (!graphicsService) {
        return;
    }
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
        // Don't fail entirely - shaders are compiled even if GPU load
        // fails.
    }
}

}  // namespace sdl3cpp::services::impl
