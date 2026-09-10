#include "services/interfaces/workflow/shader_compile_build_map.hpp"
#include "services/interfaces/workflow/shader_compile_debug_marker.hpp"

#include <string>

namespace sdl3cpp::services::impl {

std::unordered_map<std::string, ShaderPaths> BuildShaderMapWithMarkers(
    const std::shared_ptr<IShaderSystemRegistry>& shaderRegistry,
    const std::shared_ptr<ILogger>& logger) {
    if (logger) {
        logger->Info(
            "WorkflowShaderCompileStep::Execute: Building shader map "
            "from active system");
    }

    WriteShaderCompileDebugMarker(
        "test_outputs/about_to_build_shader_map.txt",
        std::string("About to call shaderRegistry_->BuildShaderMap()\n") +
            "  shaderRegistry_: " + (shaderRegistry ? "VALID" : "NULL") + "\n");

    // Build shader map using active shader system.
    const auto shaderMap = shaderRegistry->BuildShaderMap();

    WriteShaderCompileDebugMarker("test_outputs/after_build_shader_map.txt",
                                  "After shaderRegistry_->BuildShaderMap()\n"
                                  "  shaderMap.size(): " +
                                      std::to_string(shaderMap.size()) + "\n");

    if (logger) {
        logger->Info(
            "WorkflowShaderCompileStep::Execute: Shader compilation "
            "generated " +
            std::to_string(shaderMap.size()) + " shader(s)");
    }

    return shaderMap;
}

}  // namespace sdl3cpp::services::impl
