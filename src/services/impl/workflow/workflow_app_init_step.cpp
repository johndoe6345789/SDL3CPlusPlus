#include "services/interfaces/workflow/workflow_app_init_step.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowAppInitStep::WorkflowAppInitStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowAppInitStep", "Constructor", "Entry");
    }
}

std::string WorkflowAppInitStep::GetPluginId() const {
    return "app.init";
}

void WorkflowAppInitStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowAppInitStep", "Execute", "Entry");
    }

    try {
        logger_->Info("===== APPLICATION STARTING =====");

        // Get game package from context (set by main or workflow)
        std::string gamePackage = context.GetString("game_package", "standalone_cubes");
        std::string bootstrapPackage = context.GetString("bootstrap_package", "bootstrap_mac");
        std::string projectRoot = context.GetString("project_root", ".");

        logger_->Info("Game package: " + gamePackage);
        logger_->Info("Bootstrap package: " + bootstrapPackage);
        logger_->Info("Project root: " + projectRoot);

        // Verify package exists
        std::filesystem::path packageDir = projectRoot + "/packages/" + gamePackage;
        if (!std::filesystem::exists(packageDir)) {
            logger_->Error("Game package directory not found: " + packageDir.string());
            context.Set("app_initialized", false);
            return;
        }

        std::filesystem::path packageJsonPath = packageDir / "package.json";
        if (!std::filesystem::exists(packageJsonPath)) {
            logger_->Error("package.json not found: " + packageJsonPath.string());
            context.Set("app_initialized", false);
            return;
        }

        // Load package.json
        std::ifstream packageFile(packageJsonPath);
        nlohmann::json packageJson = nlohmann::json::parse(packageFile);
        std::string defaultWorkflow = packageJson.value("defaultWorkflow", "workflows/demo_gameplay.json");

        logger_->Info("Loaded package.json, defaultWorkflow: " + defaultWorkflow);

        // Store in context for downstream steps
        context.Set("package_dir", packageDir.string());
        context.Set("default_workflow", defaultWorkflow);
        context.Set("app_initialized", true);

        logger_->Info("Application initialization complete");

    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("WorkflowAppInitStep::Execute: " + std::string(e.what()));
        }
        context.Set("app_initialized", false);
    }
}

}  // namespace sdl3cpp::services::impl
