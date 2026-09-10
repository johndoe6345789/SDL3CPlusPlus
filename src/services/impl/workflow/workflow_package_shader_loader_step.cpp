#include "services/interfaces/workflow/workflow_package_shader_loader_step.hpp"
#include "services/interfaces/workflow/package_shader_load.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowPackageShaderLoaderStep::WorkflowPackageShaderLoaderStep(
    std::shared_ptr<ILogger> logger,
    const std::string& gamePackage,
    const std::filesystem::path& projectRoot)
    : logger_(std::move(logger)),
      gamePackage_(gamePackage),
      projectRoot_(ResolvePackageRoot(projectRoot)) {
    if (logger_) {
        logger_->Trace("WorkflowPackageShaderLoaderStep", "Constructor",
                      "gamePackage=" + gamePackage);
    }
}

std::string WorkflowPackageShaderLoaderStep::GetPluginId() const {
    return "shader.load_package_metadata";
}

void WorkflowPackageShaderLoaderStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    (void)step;  // Unused

    // Debug marker
    try {
        std::ofstream f("test_outputs/shader_loader_step_executed.txt");
        f << "WorkflowPackageShaderLoaderStep::Execute() was called\n";
        f.close();
    } catch (...) {}

    if (logger_) {
        logger_->Trace("WorkflowPackageShaderLoaderStep", "Execute", "Entry");
    }

    const std::string shaderBackend =
        context.Get<std::string>("shader_backend", "spirv");
    const ShaderPackageLoadResult result = LoadShaderPackageMetadata(
        projectRoot_, gamePackage_, shaderBackend, logger_);

    // shader.backend/package_json_path are only published on success,
    // matching the original: both writes happen after the try block's
    // read succeeds, so "not_found" and "error" skip them.
    context.Set<std::string>("shader.load_status", result.status);
    if (result.status == "success") {
        context.Set<std::string>("shader.backend", result.backend);
        context.Set<std::string>(
            "shader.package_json_path", result.packageJsonPath);
    } else if (result.status == "error") {
        context.Set<std::string>(
            "shader.error_message", result.errorMessage);
    }

    if (logger_) {
        logger_->Trace("WorkflowPackageShaderLoaderStep", "Execute", "Exit");
    }
}

}  // namespace sdl3cpp::services::impl
