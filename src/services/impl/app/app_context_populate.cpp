#include "services/interfaces/app/app_bootstrap.hpp"

namespace sdl3cpp::services::app {

// Load workflow variables into context, rewriting shader paths for
// platform. Shader paths in workflows default to msl/ (Mac). Bootstrap
// determines the actual backend — if not Metal, rewrite msl/ → spirv/
// and .metal → .spv.
void PopulateContextFromWorkflowVariables(
    const WorkflowDefinition& workflow, bool rewriteShaders,
    const std::shared_ptr<ILogger>& logger, WorkflowContext& outContext) {
    for (const auto& [name, var] : workflow.variables) {
        if (var.defaultValue.empty()) continue;
        if (var.type == "number") {
            try {
                outContext.Set(name, std::stod(var.defaultValue));
            } catch (...) {}
        } else if (var.type == "string") {
            std::string val = var.defaultValue;
            if (rewriteShaders &&
                val.find("/shaders/msl/") != std::string::npos) {
                // Rewrite msl path to spirv:
                // shaders/msl/foo.metal → shaders/spirv/foo.spv
                auto pos = val.find("/shaders/msl/");
                val.replace(pos, 13, "/shaders/spirv/");
                // .vert.metal → .vert.spv, .frag.metal → .frag.spv,
                // .comp.metal → .comp.spv
                auto ext = val.rfind(".metal");
                if (ext != std::string::npos) val.replace(ext, 6, ".spv");
                logger->Info("Shader rewrite: " + name + " → " + val);
            }
            outContext.Set(name, val);
        } else if (var.type == "bool") {
            outContext.Set(name, var.defaultValue == "true");
        } else {
            outContext.Set(name, var.defaultValue);
        }
    }
}

}  // namespace sdl3cpp::services::app
