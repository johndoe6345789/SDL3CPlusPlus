#include "services/interfaces/workflow/workflow_package_shader_loader_step.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

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

std::filesystem::path WorkflowPackageShaderLoaderStep::ResolvePackageRoot(
    const std::filesystem::path& projectRoot) {
    // Try to find packages directory
    std::error_code ec;
    std::vector<std::filesystem::path> candidates = {
        projectRoot / "gameengine" / "packages",
        projectRoot / "packages",
        std::filesystem::current_path() / "gameengine" / "packages",
        std::filesystem::current_path() / "packages",
    };

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate, ec)) {
            return candidate;
        }
    }

    // Return the most likely path even if it doesn't exist
    return projectRoot / "packages";
}

std::string WorkflowPackageShaderLoaderStep::GetPluginId() const {
    return "shader.load_package_metadata";
}

void WorkflowPackageShaderLoaderStep::Execute(const WorkflowStepDefinition& step,
                                              WorkflowContext& context) {
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

    try {
        // Build path to package.json
        std::filesystem::path packageJsonPath = projectRoot_ / gamePackage_ / "package.json";

        if (logger_) {
            logger_->Trace("WorkflowPackageShaderLoaderStep", "Execute",
                          "packageJsonPath=" + packageJsonPath.string());
        }

        if (!std::filesystem::exists(packageJsonPath)) {
            if (logger_) {
                logger_->Warn("WorkflowPackageShaderLoaderStep::Execute: package.json not found at " +
                             packageJsonPath.string());
            }
            context.Set<std::string>("shader.load_status", "not_found");
            return;
        }

        // Read package.json to get shader backend declaration
        std::ifstream jsonFile(packageJsonPath);
        if (!jsonFile) {
            throw std::runtime_error("Failed to open package.json");
        }

        std::string jsonContent((std::istreambuf_iterator<char>(jsonFile)),
                               std::istreambuf_iterator<char>());

        // Get shader backend from workflow context (e.g., "spirv", "msl", "glsl")
        std::string shaderBackend = context.Get<std::string>("shader_backend", "spirv");

        if (logger_) {
            logger_->Info("WorkflowPackageShaderLoaderStep::Execute: Using shader backend: " + shaderBackend);
        }

        // Store for later use by shader compilation step
        context.Set<std::string>("shader.backend", shaderBackend);
        context.Set<std::string>("shader.package_json_path", packageJsonPath.string());
        context.Set<std::string>("shader.load_status", "success");

        if (logger_) {
            logger_->Info("WorkflowPackageShaderLoaderStep::Execute: Loaded package.json with backend=" + shaderBackend);
        }

    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("WorkflowPackageShaderLoaderStep::Execute: Error: " +
                          std::string(e.what()));
        }
        context.Set<std::string>("shader.load_status", "error");
        context.Set<std::string>("shader.error_message", std::string(e.what()));
    }

    if (logger_) {
        logger_->Trace("WorkflowPackageShaderLoaderStep", "Execute", "Exit");
    }
}

}  // namespace sdl3cpp::services::impl
