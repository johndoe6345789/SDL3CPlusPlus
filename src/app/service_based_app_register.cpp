#include "app/service_based_app.hpp"
#include "events/event_bus.hpp"
#include "events/i_event_bus.hpp"
#include "services/impl/app/lifecycle_service.hpp"
#include "services/impl/diagnostics/crash_recovery_service.hpp"
#include "services/interfaces/diagnostics/probe_service.hpp"
#include "services/impl/platform/platform_service.hpp"
#include "services/interfaces/workflow/workflow_default_step_registrar.hpp"
#include "services/interfaces/workflow/workflow_definition_parser.hpp"
#include "services/interfaces/workflow/workflow_executor.hpp"
#include "services/interfaces/workflow/workflow_step_registry.hpp"
#include "services/interfaces/i_platform_service.hpp"
#include "services/interfaces/i_probe_service.hpp"
#include "services/interfaces/i_workflow_executor.hpp"
#include "services/interfaces/i_workflow_step_registry.hpp"
#include <filesystem>
#include <string>

namespace sdl3cpp::app {

void ServiceBasedApp::RegisterServices() {
    logger_->Trace("ServiceBasedApp", "RegisterServices", "", "Entering");

    // Phase 1: Core infrastructure services
    RegisterCoreServices();

    // Phase 2: Media and I/O services
    RegisterMediaServices();

    // Phase 3: Workflow loading + graphics backend selection
    services::impl::WorkflowDefinitionParser workflowParser;
    services::WorkflowDefinition workflowDefinition;
    bool workflowLoaded = false;
    try {
        // Construct workflow path from bootstrap package
        const std::filesystem::path workflowPath =
            runtimeConfig_.projectRoot / "packages" / bootstrapPackage_ / "workflows" / "boot.json";
        logger_->Trace("ServiceBasedApp", "RegisterServices",
                      "bootstrapPackage=" + bootstrapPackage_ +
                      " workflowPath=" + workflowPath.string(),
                      "Loading boot workflow from package");
        workflowDefinition = workflowParser.ParseFile(workflowPath);
        workflowLoaded = true;

        // Inject CLI arguments into workflow definition for template resolution
        workflowDefinition.cliArgs["bootstrap"] = bootstrapPackage_;
        workflowDefinition.cliArgs["game"] = gamePackage_;
        logger_->Trace("ServiceBasedApp", "RegisterServices",
                      "cliArgs.bootstrap=" + bootstrapPackage_ +
                      " cliArgs.game=" + gamePackage_,
                      "CLI arguments injected - accessible via {{ $cli.* }} in workflows");

        // Trace log workflow variables
        logger_->Trace("ServiceBasedApp", "RegisterServices",
                      "variableCount=" + std::to_string(workflowDefinition.variables.size()),
                      "Workflow variables loaded from n8n schema");
        for (const auto& [name, var] : workflowDefinition.variables) {
            logger_->Trace("ServiceBasedApp", "RegisterServices",
                          "variable." + name + "=" + var.defaultValue +
                          " type=" + var.type +
                          " required=" + (var.required ? "true" : "false"),
                          "Workflow variable: " + var.description);
        }
    } catch (const std::exception& e) {
        logger_->Warn("ServiceBasedApp::RegisterServices: Failed to load workflow template: " +
                      std::string(e.what()));
        logger_->Trace("ServiceBasedApp", "RegisterServices",
                      "workflowLoaded=false",
                      "Workflow loading failed - will use fallback defaults");
    }
    // Phase 4: Determine graphics backend from workflow variables
    std::string backendName = "sdl3_gpu";  // Default fallback
    if (workflowLoaded && !workflowDefinition.variables.empty()) {
        auto it = workflowDefinition.variables.find("graphicsBackend");
        if (it != workflowDefinition.variables.end()) {
            backendName = it->second.defaultValue;
            logger_->Trace("ServiceBasedApp", "RegisterServices",
                          "workflow.graphicsBackend=" + backendName,
                          "Reading graphics backend from workflow variables");
        }
    }

    // Phase 5: Graphics pipeline services (BEFORE workflow registration so bootstrap steps get services)
    RegisterGraphicsServices(backendName);

    // Phase 6: Register bootstrap workflow steps (NOW with graphics services available)
    if (workflowLoaded) {
        services::impl::WorkflowDefaultStepRegistrar workflowRegistrar(
            registry_.GetService<services::ILogger>(),
            registry_.GetService<services::IProbeService>(),
            registry_.GetService<services::IGraphicsService>(),
            registry_.GetService<services::IWindowService>(),
            nullptr,
            registry_.GetService<services::IInputService>());
        workflowRegistrar.RegisterUsedSteps(
            workflowDefinition,
            registry_.GetService<services::IWorkflowStepRegistry>());

        // Store bootstrap workflow for execution during Run()
        bootstrapWorkflow_ = workflowDefinition;
        bootstrapWorkflowLoaded_ = true;
    }

    logger_->Trace("ServiceBasedApp", "RegisterServices", "", "Exiting");
}

void ServiceBasedApp::RegisterCoreServices() {
    // Logger service already registered in constructor

    // Crash recovery service (needed early for crash detection)
    registry_.RegisterService<services::ICrashRecoveryService, services::impl::CrashRecoveryService>(
        registry_.GetService<services::ILogger>(),
        runtimeConfig_.crashRecovery);

    // Lifecycle service
    registry_.RegisterService<services::ILifecycleService, services::impl::LifecycleService>(
        registry_,
        registry_.GetService<services::ILogger>());

    // Platform service (needed for SDL error enrichment)
    registry_.RegisterService<services::IPlatformService, services::impl::PlatformService>(
        registry_.GetService<services::ILogger>());

    // Event bus (needed by window service)
    registry_.RegisterService<events::IEventBus, events::EventBus>();

    // Probe service (structured diagnostics)
    registry_.RegisterService<services::IProbeService, services::impl::ProbeService>(
        registry_.GetService<services::ILogger>());

    // Workflow step registry + executor (declarative boot/frame pipelines)
    registry_.RegisterService<services::IWorkflowStepRegistry, services::impl::WorkflowStepRegistry>();
    registry_.RegisterService<services::IWorkflowExecutor, services::impl::WorkflowExecutor>(
        registry_.GetService<services::IWorkflowStepRegistry>(),
        registry_.GetService<services::ILogger>());
}

}  // namespace sdl3cpp::app
