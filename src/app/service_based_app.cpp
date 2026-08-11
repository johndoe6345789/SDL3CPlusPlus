#include "app/service_based_app.hpp"
#include "services/interfaces/diagnostics/logger_service.hpp"
#include "services/interfaces/workflow/workflow_executor.hpp"
#include "services/interfaces/workflow/workflow_step_registry.hpp"
#include "services/interfaces/workflow_registrar.hpp"
#include <iostream>
#include <stdexcept>
#include <utility>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace sdl3cpp::app {

ServiceBasedApp::ServiceBasedApp(services::RuntimeConfig runtimeConfig,
                               services::LogLevel logLevel,
                               const std::string& bootstrapPackage,
                               const std::string& gamePackage)
    : runtimeConfig_(std::move(runtimeConfig)),
      bootstrapPackage_(bootstrapPackage),
      gamePackage_(gamePackage) {
    // Register logger service first
    registry_.RegisterService<services::ILogger, services::impl::LoggerService>();
    logger_ = registry_.GetService<services::ILogger>();
    if (logger_) {
        logger_->SetLevel(logLevel);
        logger_->EnableConsoleOutput(false);
        logger_->Info("ServiceBasedApp constructor starting");
    }
}

void ServiceBasedApp::ConfigureLogging(services::LogLevel level, bool enableConsole, const std::string& outputFile) {
    if (logger_) {
        logger_->SetLevel(level);
        logger_->EnableConsoleOutput(enableConsole);
        if (!outputFile.empty()) {
            logger_->SetOutputFile(outputFile);
        }
    }
}

void ServiceBasedApp::Run() {
    if (logger_) {
        logger_->Info("Run: Application starting");
        logger_->Info("Bootstrap=" + bootstrapPackage_ + " Game=" + gamePackage_);
    }

    try {
        // Load game package workflows
        std::filesystem::path packageDir = runtimeConfig_.projectRoot / "packages" / gamePackage_;
        std::filesystem::path packageJsonPath = packageDir / "package.json";

        if (!std::filesystem::exists(packageJsonPath)) {
            if (logger_) {
                logger_->Error("Run: Package not found: " + packageJsonPath.string());
            }
            return;
        }

        if (logger_) {
            logger_->Info("Run: Loading package from " + packageDir.string());
        }

        // Read package.json to find default workflow
        std::ifstream packageFile(packageJsonPath);
        nlohmann::json packageJson = nlohmann::json::parse(packageFile);
        std::string defaultWorkflow = packageJson.value("defaultWorkflow", "");

        if (defaultWorkflow.empty()) {
            if (logger_) {
                logger_->Error("Run: No defaultWorkflow specified in package.json");
            }
            return;
        }

        if (logger_) {
            logger_->Info("Run: Default workflow: " + defaultWorkflow);
        }

        // Load workflow JSON
        std::filesystem::path workflowPath = packageDir / defaultWorkflow;
        if (!std::filesystem::exists(workflowPath)) {
            if (logger_) {
                logger_->Error("Run: Workflow not found: " + workflowPath.string());
            }
            return;
        }

        std::ifstream workflowFile(workflowPath);
        nlohmann::json workflowJson = nlohmann::json::parse(workflowFile);

        if (logger_) {
            logger_->Info("Run: Loaded workflow: " + workflowJson.value("name", ""));
        }

        // For now, just log that we loaded it
        // Full workflow execution requires all 16 steps to be implemented
        if (logger_) {
            logger_->Info("Run: Workflow loaded successfully (execution requires step implementations)");
            logger_->Info("Run: Complete");
        }

    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("Run: Exception: " + std::string(e.what()));
        }
        throw;
    }
}

}  // namespace sdl3cpp::app
