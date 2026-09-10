#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

#include <SDL3/SDL_main.h>

#include "services/interfaces/app/app_bootstrap.hpp"
#include "services/interfaces/workflow/workflow_definition_parser.hpp"

using sdl3cpp::services::WorkflowContext;
namespace app = sdl3cpp::services::app;

int main(int argc, char** argv) {
    SDL_SetMainReady();

    try {
        app::CliOptions options;
        if (!app::ParseCliArgs(argc, argv, options)) {
            return 1;
        }

        if (!app::RequirePackage(options.projectRoot, "bootstrap", "bootloader",
                                 options.bootstrapPackage)) {
            return 1;
        }
        if (!app::RequirePackage(options.projectRoot, "game", "game",
                                 options.gamePackage)) {
            return 1;
        }

        auto logger =
            app::CreateAppLogger(options.projectRoot, options.traceEnabled);
        app::WorkflowRuntime runtime = app::BuildWorkflowRuntime(logger);

        // Create context with CLI arguments.
        WorkflowContext appContext;
        appContext.Set("game_package", options.gamePackage);
        appContext.Set("bootstrap_package", options.bootstrapPackage);
        appContext.Set("project_root", options.projectRoot.string());
        appContext.Set("max_frames", 600.0);

        std::string defaultWorkflow = app::LoadDefaultWorkflowPath(
            options.projectRoot, options.gamePackage, logger);
        std::string shaderDir = app::DetermineShaderBackend(
            options.projectRoot, options.bootstrapPackage);
        appContext.Set<std::string>("shader_backend", shaderDir);
        logger->Info("Shader backend: " + shaderDir +
                     " (bootstrap: " + options.bootstrapPackage + ")");

        // Load and execute the default workflow.
        std::filesystem::path mainWorkflowPath =
            options.projectRoot / "packages" / options.gamePackage /
            defaultWorkflow;
        if (!std::filesystem::exists(mainWorkflowPath)) {
            logger->Error("Workflow not found: " + mainWorkflowPath.string());
            return EXIT_FAILURE;
        }

        logger->Info("Loading workflow: " + mainWorkflowPath.string());
        sdl3cpp::services::impl::WorkflowDefinitionParser parser(logger);
        auto mainWorkflow         = parser.ParseFile(mainWorkflowPath);
        const bool rewriteShaders = (shaderDir != "msl");
        app::PopulateContextFromWorkflowVariables(mainWorkflow, rewriteShaders,
                                                  logger, appContext);

        logger->Info("Executing main workflow (" +
                     std::to_string(mainWorkflow.steps.size()) + " steps)");
        runtime.executor->Execute(mainWorkflow, appContext);

        logger->Info("===== APPLICATION COMPLETE =====");

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
