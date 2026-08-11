#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>
#include <SDL3/SDL_main.h>

#include "services/interfaces/diagnostics/logger_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/workflow_executor.hpp"
#include "services/interfaces/workflow/workflow_step_registry.hpp"
#include "services/interfaces/workflow_registrar.hpp"
#include "services/interfaces/workflow/workflow_definition_parser.hpp"
#include "services/interfaces/workflow/workflow_app_init_step.hpp"
#include "services/interfaces/workflow/workflow_load_workflow_step.hpp"

int main(int argc, char** argv) {
    SDL_SetMainReady();

    try {
        // Parse command line (inline)
        std::string gamePackage = "standalone_cubes";
        std::string bootstrapPackage = "bootstrap_mac";
        std::filesystem::path projectRoot = std::filesystem::current_path();

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--game" && i + 1 < argc) {
                gamePackage = argv[++i];
            } else if (arg == "--bootstrap" && i + 1 < argc) {
                bootstrapPackage = argv[++i];
            } else if (arg == "--project-root" && i + 1 < argc) {
                projectRoot = argv[++i];
            }
        }

        // Create logger
        auto logger = std::make_shared<sdl3cpp::services::impl::LoggerService>();
        logger->EnableConsoleOutput(false);
        std::filesystem::path logPath = projectRoot / "sdl3_app.log";
        logger->SetOutputFile(logPath.string());

        // Create workflow infrastructure
        auto registry = std::make_shared<sdl3cpp::services::impl::WorkflowStepRegistry>();
        auto registrar = std::make_unique<sdl3cpp::services::impl::WorkflowRegistrar>(logger);
        registrar->RegisterSteps(registry);

        // Register application lifecycle steps
        registry->RegisterStep(std::make_shared<sdl3cpp::services::impl::WorkflowAppInitStep>(logger));
        registry->RegisterStep(std::make_shared<sdl3cpp::services::impl::WorkflowLoadWorkflowStep>(logger));

        auto executor = std::make_shared<sdl3cpp::services::impl::WorkflowExecutor>(registry, logger);

        // Register executor-dependent steps (control.loop.while, workflow.execute)
        registrar->RegisterExecutorSteps(registry, executor);

        // Create context with CLI arguments
        sdl3cpp::services::WorkflowContext appContext;
        appContext.Set("game_package", gamePackage);
        appContext.Set("bootstrap_package", bootstrapPackage);
        appContext.Set("project_root", projectRoot.string());
        appContext.Set("max_frames", 600.0);

        // Load package.json to get defaultWorkflow
        std::filesystem::path packageJsonPath = projectRoot / "packages" / gamePackage / "package.json";
        std::string defaultWorkflow = "workflows/main.json";  // fallback

        if (std::filesystem::exists(packageJsonPath)) {
            std::ifstream packageFile(packageJsonPath);
            if (packageFile.is_open()) {
                try {
                    nlohmann::json packageJson;
                    packageFile >> packageJson;
                    if (packageJson.contains("defaultWorkflow")) {
                        defaultWorkflow = packageJson["defaultWorkflow"].get<std::string>();
                        logger->Info("Loaded package.json, defaultWorkflow: " + defaultWorkflow);
                    }
                } catch (const std::exception& e) {
                    logger->Warn("Failed to parse package.json: " + std::string(e.what()));
                }
            }
        }

        // Determine shader backend from bootstrap package
        std::string shaderDir = "msl";  // default (Mac)
        {
            std::filesystem::path bootPkgPath = projectRoot / "packages" / bootstrapPackage / "package.json";
            if (std::filesystem::exists(bootPkgPath)) {
                std::ifstream bootFile(bootPkgPath);
                nlohmann::json bootJson;
                bootFile >> bootJson;
                if (bootJson.contains("config") && bootJson["config"].contains("renderer")) {
                    std::string renderer = bootJson["config"]["renderer"].get<std::string>();
                    if (renderer != "metal") shaderDir = "spirv";
                }
            }
        }
        appContext.Set<std::string>("shader_backend", shaderDir);
        logger->Info("Shader backend: " + shaderDir + " (bootstrap: " + bootstrapPackage + ")");

        // Load and execute the default workflow
        std::filesystem::path mainWorkflowPath = projectRoot / "packages" / gamePackage / defaultWorkflow;
        if (!std::filesystem::exists(mainWorkflowPath)) {
            logger->Error("Workflow not found: " + mainWorkflowPath.string());
            return EXIT_FAILURE;
        }

        logger->Info("Loading workflow: " + mainWorkflowPath.string());
        sdl3cpp::services::impl::WorkflowDefinitionParser parser(logger);
        auto mainWorkflow = parser.ParseFile(mainWorkflowPath);

        // Load workflow variables into context, rewriting shader paths for platform
        // Shader paths in workflows default to msl/ (Mac). Bootstrap determines the
        // actual backend — if not Metal, rewrite msl/ → spirv/ and .metal → .spv
        const bool rewriteShaders = (shaderDir != "msl");

        for (const auto& [name, var] : mainWorkflow.variables) {
            if (var.defaultValue.empty()) continue;
            if (var.type == "number") {
                try {
                    appContext.Set(name, std::stod(var.defaultValue));
                } catch (...) {}
            } else if (var.type == "string") {
                std::string val = var.defaultValue;
                if (rewriteShaders && val.find("/shaders/msl/") != std::string::npos) {
                    // Rewrite msl path to spirv: shaders/msl/foo.metal → shaders/spirv/foo.spv
                    auto pos = val.find("/shaders/msl/");
                    val.replace(pos, 13, "/shaders/spirv/");
                    // .vert.metal → .vert.spv, .frag.metal → .frag.spv, .comp.metal → .comp.spv
                    auto ext = val.rfind(".metal");
                    if (ext != std::string::npos) val.replace(ext, 6, ".spv");
                    logger->Info("Shader rewrite: " + name + " → " + val);
                }
                appContext.Set(name, val);
            } else if (var.type == "bool") {
                appContext.Set(name, var.defaultValue == "true");
            } else {
                appContext.Set(name, var.defaultValue);
            }
        }

        logger->Info("Executing main workflow (" + std::to_string(mainWorkflow.steps.size()) + " steps)");
        executor->Execute(mainWorkflow, appContext);

        logger->Info("===== APPLICATION COMPLETE =====");

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
