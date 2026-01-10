#include "service_based_app.hpp"
#include "events/event_bus.hpp"
#include "events/i_event_bus.hpp"
#include "services/interfaces/i_window_service.hpp"
#include "services/interfaces/i_graphics_service.hpp"
#include "services/interfaces/i_application_loop_service.hpp"
#include "services/interfaces/i_lifecycle_service.hpp"
#include "services/impl/config/json_config_service.hpp"
#include "services/impl/config/config_compiler_service.hpp"
#include "services/impl/app/lifecycle_service.hpp"
#include "services/impl/app/application_loop_service.hpp"
#include "services/impl/render/render_coordinator_service.hpp"
#include "services/impl/render/render_graph_service.hpp"
#include "services/impl/platform/platform_service.hpp"
#include "services/impl/diagnostics/probe_service.hpp"
#include "services/impl/platform/sdl_window_service.hpp"
#include "services/impl/input/sdl_input_service.hpp"
#include "services/impl/scene/ecs_service.hpp"
#include "services/impl/graphics/graphics_service.hpp"
#include "services/impl/graphics/bgfx_graphics_backend.hpp"
#include "services/impl/shader/shader_system_registry.hpp"
#include "services/impl/scene/physics_bridge_service.hpp"
#include "services/impl/scene/mesh_service.hpp"
#include "services/impl/scene/scene_service.hpp"
#include "services/impl/audio/sdl_audio_service.hpp"
#include "services/impl/gui/null_gui_service.hpp"
#if !defined(SDL3CPP_ENABLE_VITA)
#include "services/impl/gui/bgfx_gui_service.hpp"
#endif
#include "services/impl/scene/bullet_physics_service.hpp"
#include "services/impl/diagnostics/crash_recovery_service.hpp"
#include "services/impl/diagnostics/logger_service.hpp"
#include "services/impl/shader/pipeline_compiler_service.hpp"
#include "services/impl/diagnostics/validation_tour_service.hpp"
#include "services/impl/soundboard/soundboard_state_service.hpp"
#include "services/impl/workflow/workflow_default_step_registrar.hpp"
#include "services/impl/workflow/workflow_definition_parser.hpp"
#include "services/impl/workflow/workflow_executor.hpp"
#include "services/impl/workflow/workflow_step_registry.hpp"
#include "services/impl/workflow/frame/frame_workflow_service.hpp"
#include "services/interfaces/i_platform_service.hpp"
#include "services/interfaces/i_probe_service.hpp"
#include "services/interfaces/i_render_graph_service.hpp"
#include "services/interfaces/i_shader_system_registry.hpp"
#include "services/interfaces/i_validation_tour_service.hpp"
#include "services/interfaces/i_workflow_executor.hpp"
#include "services/interfaces/i_workflow_step_registry.hpp"
#include "services/interfaces/i_frame_workflow_service.hpp"
#include <iostream>
#include <filesystem>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::app {

ServiceBasedApp::ServiceBasedApp(services::RuntimeConfig runtimeConfig, services::LogLevel logLevel)
    : runtimeConfig_(std::move(runtimeConfig)) {
    // Register logger service first
    registry_.RegisterService<services::ILogger, services::impl::LoggerService>();
    logger_ = registry_.GetService<services::ILogger>();
    if (logger_) {
        logger_->SetLevel(logLevel);
    }
    
    logger_->Trace("ServiceBasedApp", "ServiceBasedApp",
                   "projectRoot=" + runtimeConfig_.projectRoot.string(),
                   "constructor starting");

    try {
        logger_->Info("ServiceBasedApp::ServiceBasedApp: Setting up SDL");
        SetupSDL();
        logger_->Info("ServiceBasedApp::ServiceBasedApp: Registering services");
        RegisterServices();
        
        // Get and initialize crash recovery service
        crashRecoveryService_ = registry_.GetService<services::ICrashRecoveryService>();
        if (crashRecoveryService_) {
            crashRecoveryService_->Initialize();
        }
        
        logger_->Info("ServiceBasedApp::ServiceBasedApp: Resolving lifecycle services");

        lifecycleService_ = registry_.GetService<services::ILifecycleService>();
        applicationLoopService_ = registry_.GetService<services::IApplicationLoopService>();
        renderCoordinatorService_ = registry_.GetService<services::IRenderCoordinatorService>();
        
        logger_->Info("ServiceBasedApp::ServiceBasedApp: constructor completed");
    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("ServiceBasedApp::ServiceBasedApp: Failed to initialize ServiceBasedApp: " + std::string(e.what()));
        } else {
            // Fallback to console if logger not available
            std::cerr << "ServiceBasedApp::ServiceBasedApp: Failed to initialize ServiceBasedApp: " << e.what() << std::endl;
        }
        throw;
    }
}

ServiceBasedApp::~ServiceBasedApp() {
    logger_->Trace("ServiceBasedApp", "~ServiceBasedApp", "", "Entering");

    // Shutdown crash recovery service
    if (crashRecoveryService_) {
        crashRecoveryService_->Shutdown();
    }

    renderCoordinatorService_.reset();
    applicationLoopService_.reset();
    lifecycleService_.reset();

    logger_->Trace("ServiceBasedApp", "~ServiceBasedApp", "", "Exiting");
}

void ServiceBasedApp::Run() {
    logger_->Trace("ServiceBasedApp", "Run", "", "Entering");

    try {
        // Initialize all services
        lifecycleService_->InitializeAll();

        // Create the window
        auto windowService = registry_.GetService<services::IWindowService>();
        auto configService = registry_.GetService<services::IConfigService>();
        if (windowService) {
            services::WindowConfig config;
            if (configService) {
                config.width = configService->GetWindowWidth();
                config.height = configService->GetWindowHeight();
                config.title = configService->GetWindowTitle();
                config.mouseGrab = configService->GetMouseGrabConfig();
            } else {
                config.width = runtimeConfig_.width;
                config.height = runtimeConfig_.height;
                config.title = runtimeConfig_.windowTitle;
                config.mouseGrab = runtimeConfig_.mouseGrab;
            }
            config.resizable = true;
            windowService->CreateWindow(config);
        }

        // Initialize graphics after window is created
        auto graphicsService = registry_.GetService<services::IGraphicsService>();
        if (graphicsService && windowService) {
            services::GraphicsConfig graphicsConfig;
            graphicsService->InitializeDevice(windowService->GetNativeHandle(), graphicsConfig);
            graphicsService->InitializeSwapchain();
        }

        // Run the main application loop with crash recovery
        if (crashRecoveryService_) {
            constexpr int kMainLoopTimeoutMs = 24 * 60 * 60 * 1000; // Safety net; heartbeat monitor handles hangs.
            bool success = crashRecoveryService_->ExecuteWithTimeout(
                [this]() { applicationLoopService_->Run(); },
                kMainLoopTimeoutMs,
                "Main Application Loop"
            );

            if (!success) {
                logger_->Warn("ServiceBasedApp::Run: Main loop stopped by crash recovery, attempting recovery");
                if (crashRecoveryService_->AttemptRecovery()) {
                    logger_->Info("ServiceBasedApp::Run: Recovery successful, restarting main loop");
                    applicationLoopService_->Run(); // Try again
                }
            }
        } else {
            // Fallback if no crash recovery service
            applicationLoopService_->Run();
        }

        // Shutdown all services
        lifecycleService_->ShutdownAll();

        logger_->Trace("ServiceBasedApp", "Run", "", "Exiting");

    } catch (const std::exception& e) {
        logger_->Error("ServiceBasedApp::Run: Application error: " + std::string(e.what()));
        
        // Attempt recovery on exception
        if (crashRecoveryService_ && crashRecoveryService_->AttemptRecovery()) {
            logger_->Info("ServiceBasedApp::Run: Recovered from exception");
        } else {
            throw;
        }
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

void ServiceBasedApp::SetupSDL() {
    logger_->Trace("ServiceBasedApp", "SetupSDL", "", "Entering");

    // SDL initialization is handled by the window service
    // Don't initialize SDL here to avoid double initialization

    logger_->Trace("ServiceBasedApp", "SetupSDL", "", "Exiting");
}

void ServiceBasedApp::RegisterServices() {
    logger_->Trace("ServiceBasedApp", "RegisterServices", "", "Entering");

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
    services::impl::WorkflowDefinitionParser workflowParser;
    services::WorkflowDefinition workflowDefinition;
    bool workflowLoaded = false;
    try {
        const std::filesystem::path workflowPath =
            std::filesystem::current_path() / "config" / "workflows" / "templates" / "boot_default.json";
        workflowDefinition = workflowParser.ParseFile(workflowPath);
        workflowLoaded = true;
    } catch (const std::exception& e) {
        logger_->Warn("ServiceBasedApp::RegisterServices: Failed to load workflow template: " +
                      std::string(e.what()));
    }
    if (workflowLoaded) {
        services::impl::WorkflowDefaultStepRegistrar workflowRegistrar(
            registry_.GetService<services::ILogger>(),
            registry_.GetService<services::IProbeService>());
        workflowRegistrar.RegisterUsedSteps(
            workflowDefinition,
            registry_.GetService<services::IWorkflowStepRegistry>());
    }

    // Configuration service
    registry_.RegisterService<services::IConfigService, services::impl::JsonConfigService>(
        registry_.GetService<services::ILogger>(),
        runtimeConfig_,
        registry_.GetService<services::IProbeService>());
    auto configService = registry_.GetService<services::IConfigService>();

    // Validation tour service (startup visual checks)
    registry_.RegisterService<services::IValidationTourService, services::impl::ValidationTourService>(
        configService,
        registry_.GetService<services::IProbeService>(),
        registry_.GetService<services::ILogger>());

    // Render graph service (DAG build + scheduling)
    registry_.RegisterService<services::IRenderGraphService, services::impl::RenderGraphService>(
        registry_.GetService<services::ILogger>(),
        registry_.GetService<services::IProbeService>());

    // Config compiler service (JSON -> IR)
    registry_.RegisterService<services::IConfigCompilerService, services::impl::ConfigCompilerService>(
        registry_.GetService<services::IConfigService>(),
        registry_.GetService<services::IRenderGraphService>(),
        registry_.GetService<services::IProbeService>(),
        registry_.GetService<services::ILogger>());
    // ECS service (entt registry)
    registry_.RegisterService<services::IEcsService, services::impl::EcsService>(
        registry_.GetService<services::ILogger>());

    // Pipeline compiler service (bgfx shader compilation)
    registry_.RegisterService<services::IPipelineCompilerService, services::impl::PipelineCompilerService>(
        registry_.GetService<services::ILogger>());

    // Window service
    registry_.RegisterService<services::IWindowService, services::impl::SdlWindowService>(
        registry_.GetService<services::ILogger>(),
        registry_.GetService<services::IPlatformService>(),
        registry_.GetService<events::IEventBus>());

    // Input service
    registry_.RegisterService<services::IInputService, services::impl::SdlInputService>(
        registry_.GetService<events::IEventBus>(),
        registry_.GetService<services::IConfigService>(),
        registry_.GetService<services::ILogger>());

    // Audio service
    registry_.RegisterService<services::IAudioService, services::impl::SdlAudioService>(
        registry_.GetService<services::ILogger>());

    registry_.RegisterService<services::ISoundboardStateService, services::impl::SoundboardStateService>(
        registry_.GetService<services::ILogger>());

    registry_.RegisterService<services::IMeshService, services::impl::MeshService>(
        registry_.GetService<services::IConfigService>(),
        registry_.GetService<services::ILogger>());

    registry_.RegisterService<services::IFrameWorkflowService, services::impl::FrameWorkflowService>(
        registry_.GetService<services::ILogger>(),
        registry_.GetService<services::IConfigService>(),
        registry_.GetService<services::IAudioService>(),
        registry_.GetService<services::IInputService>(),
        registry_.GetService<services::IMeshService>(),
        registry_.GetService<services::IPhysicsService>(),
        registry_.GetService<services::ISceneService>(),
        registry_.GetService<services::IRenderCoordinatorService>(),
        registry_.GetService<services::IValidationTourService>(),
        registry_.GetService<services::ISoundboardStateService>());

    // Physics bridge services
    registry_.RegisterService<services::IPhysicsBridgeService, services::impl::PhysicsBridgeService>(
        registry_.GetService<services::ILogger>());

    // Shader system registry (pluggable shader system selection)
    registry_.RegisterService<services::IShaderSystemRegistry, services::impl::ShaderSystemRegistry>(
        registry_.GetService<services::IConfigService>(),
        registry_.GetService<services::IConfigCompilerService>(),
        registry_.GetService<services::ILogger>());

    auto graphicsBackend = std::make_shared<services::impl::BgfxGraphicsBackend>(
        registry_.GetService<services::IConfigService>(),
        registry_.GetService<services::IPlatformService>(),
        registry_.GetService<services::ILogger>(),
        registry_.GetService<services::IPipelineCompilerService>(),
        registry_.GetService<services::IProbeService>());

    // Graphics service (facade)
    registry_.RegisterService<services::IGraphicsService, services::impl::GraphicsService>(
        registry_.GetService<services::ILogger>(),
        graphicsBackend,
        registry_.GetService<services::IWindowService>());

    // Scene service
    registry_.RegisterService<services::ISceneService, services::impl::SceneService>(
        registry_.GetService<services::IEcsService>(),
        registry_.GetService<services::ILogger>(),
        registry_.GetService<services::IProbeService>());

    // GUI service
#if defined(SDL3CPP_ENABLE_VITA)
    registry_.RegisterService<services::IGuiService, services::impl::NullGuiService>(
        registry_.GetService<services::ILogger>());
#else
    registry_.RegisterService<services::IGuiService, services::impl::BgfxGuiService>(
        registry_.GetService<services::IConfigService>(),
        registry_.GetService<services::ILogger>(),
        registry_.GetService<services::IPipelineCompilerService>());
#endif

    // Physics service
    registry_.RegisterService<services::IPhysicsService, services::impl::BulletPhysicsService>(
        registry_.GetService<services::ILogger>());

    // Render coordinator service
    registry_.RegisterService<services::IRenderCoordinatorService, services::impl::RenderCoordinatorService>(
        registry_.GetService<services::ILogger>(),
        registry_.GetService<services::IConfigCompilerService>(),
        registry_.GetService<services::IGraphicsService>(),
        registry_.GetService<services::IShaderSystemRegistry>(),
        registry_.GetService<services::IGuiService>(),
        registry_.GetService<services::ISceneService>(),
        registry_.GetService<services::IValidationTourService>());

    // Application loop service
    registry_.RegisterService<services::IApplicationLoopService, services::impl::ApplicationLoopService>(
        registry_.GetService<services::ILogger>(),
        registry_.GetService<services::IWindowService>(),
        registry_.GetService<events::IEventBus>(),
        registry_.GetService<services::IInputService>(),
        registry_.GetService<services::IPhysicsService>(),
        registry_.GetService<services::ISceneService>(),
        registry_.GetService<services::IRenderCoordinatorService>(),
        registry_.GetService<services::IAudioService>(),
        registry_.GetService<services::IFrameWorkflowService>(),
        registry_.GetService<services::ICrashRecoveryService>());

    logger_->Trace("ServiceBasedApp", "RegisterServices", "", "Exiting");
}

}  // namespace sdl3cpp::app
