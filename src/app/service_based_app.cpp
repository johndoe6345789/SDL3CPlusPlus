#include "service_based_app.hpp"
#include "logging/logger.hpp"
#include "events/event_bus.hpp"
#include "services/interfaces/i_window_service.hpp"
#include "services/interfaces/i_graphics_service.hpp"
#include "services/impl/json_config_service.hpp"
#include "services/impl/sdl_window_service.hpp"
#include "services/impl/sdl_input_service.hpp"
#include "services/impl/vulkan_device_service.hpp"
#include "services/impl/swapchain_service.hpp"
#include "services/impl/pipeline_service.hpp"
#include "services/impl/buffer_service.hpp"
#include "services/impl/render_command_service.hpp"
#include "services/impl/graphics_service.hpp"
#include "services/impl/lua_script_service.hpp"
#include "services/impl/scene_service.hpp"
#include "services/impl/sdl_audio_service.hpp"
#include "services/impl/vulkan_gui_service.hpp"
#include "services/impl/bullet_physics_service.hpp"
#include "services/impl/crash_recovery_service.hpp"
#include "services/impl/logger_service.hpp"
#include <stdexcept>

namespace sdl3cpp::app {

ServiceBasedApp::ServiceBasedApp(const std::filesystem::path& scriptPath)
    : scriptPath_(scriptPath) {
    // Register logger service first
    registry_.RegisterService<services::ILogger, services::impl::LoggerService>();
    logger_ = registry_.GetService<services::ILogger>();
    
    logger_->Trace("ServiceBasedApp", "ServiceBasedApp", "scriptPath=" + scriptPath_.string(), "constructor starting");

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
        
        logger_->Info("ServiceBasedApp::ServiceBasedApp: Creating controllers");

        lifecycleController_ = std::make_unique<controllers::LifecycleController>(registry_);
        applicationController_ = std::make_unique<controllers::ApplicationController>(registry_);
        
        logger_->Info("ServiceBasedApp::ServiceBasedApp: constructor completed");
    } catch (const std::exception& e) {
        logging::Logger::GetInstance().Error("ServiceBasedApp::ServiceBasedApp: Failed to initialize ServiceBasedApp: " + std::string(e.what()));
        throw;
    }
}

ServiceBasedApp::~ServiceBasedApp() {
    logger_->Trace("ServiceBasedApp", "~ServiceBasedApp", "", "Entering");

    // Shutdown crash recovery service
    if (crashRecoveryService_) {
        crashRecoveryService_->Shutdown();
    }

    applicationController_.reset();
    lifecycleController_.reset();

    logger_->Trace("ServiceBasedApp", "~ServiceBasedApp", "", "Exiting");
}

void ServiceBasedApp::Run() {
    logger_->Trace("ServiceBasedApp", "Run", "", "Entering");

    try {
        // Initialize all services
        lifecycleController_->InitializeAll();

        // Create the window
        auto windowService = registry_.GetService<services::IWindowService>();
        if (windowService) {
            services::WindowConfig config;
            config.width = 1024;
            config.height = 768;
            config.title = "SDL3 + Vulkan Application";
            config.resizable = true;
            windowService->CreateWindow(config);
        }

        // Initialize graphics after window is created
        auto graphicsService = registry_.GetService<services::IGraphicsService>();
        if (graphicsService && windowService) {
            services::GraphicsConfig graphicsConfig;
            graphicsConfig.deviceExtensions = {"VK_KHR_swapchain"};
            graphicsConfig.enableValidationLayers = false;
            graphicsService->InitializeDevice(windowService->GetNativeHandle(), graphicsConfig);
            graphicsService->InitializeSwapchain();
        }

        // Connect services that depend on each other
        auto scriptService = registry_.GetService<services::IScriptService>();
        auto audioService = registry_.GetService<services::IAudioService>();
        if (scriptService && audioService) {
            // The script service needs access to the audio player for Lua audio commands
            // This is a bit of a hack - ideally the audio service would provide an interface
            // But for now, we'll get the audio player from the impl
            auto audioServiceImpl = std::dynamic_pointer_cast<services::impl::SdlAudioService>(audioService);
            if (audioServiceImpl) {
                // We need to access the private audioPlayer_ - this is not ideal
                // TODO: Refactor to provide proper interface
            }
        }

        // Run the main application loop with crash recovery
        if (crashRecoveryService_) {
            bool success = crashRecoveryService_->ExecuteWithTimeout(
                [this]() { applicationController_->Run(); },
                30000, // 30 second timeout for the main loop
                "Main Application Loop"
            );

            if (!success) {
                logger_->Warn("ServiceBasedApp::Run: Main loop timed out, attempting recovery");
                if (crashRecoveryService_->AttemptRecovery()) {
                    logger_->Info("ServiceBasedApp::Run: Recovery successful, restarting main loop");
                    applicationController_->Run(); // Try again
                }
            }
        } else {
            // Fallback if no crash recovery service
            applicationController_->Run();
        }

        // Shutdown all services
        lifecycleController_->ShutdownAll();

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
        registry_.GetService<services::ILogger>());

    // Event bus (needed by window service)
    registry_.RegisterService<events::EventBus, events::EventBus>();

    // Configuration service
    services::impl::RuntimeConfig runtimeConfig;
    runtimeConfig.scriptPath = scriptPath_;
    registry_.RegisterService<services::IConfigService, services::impl::JsonConfigService>(
        registry_.GetService<services::ILogger>(), runtimeConfig);

    // Window service
    registry_.RegisterService<services::IWindowService, services::impl::SdlWindowService>(
        registry_.GetService<services::ILogger>(),
        registry_.GetService<events::EventBus>());

    // Input service
    registry_.RegisterService<services::IInputService, services::impl::SdlInputService>(
        registry_.GetService<events::EventBus>(),
        registry_.GetService<services::ILogger>());

    // Vulkan device service
    registry_.RegisterService<services::IVulkanDeviceService, services::impl::VulkanDeviceService>(
        registry_.GetService<services::ILogger>());

    // Swapchain service
    registry_.RegisterService<services::ISwapchainService, services::impl::SwapchainService>(
        registry_.GetService<services::IVulkanDeviceService>(),
        registry_.GetService<events::EventBus>(),
        registry_.GetService<services::ILogger>());

    // Pipeline service
    registry_.RegisterService<services::IPipelineService, services::impl::PipelineService>(
        registry_.GetService<services::IVulkanDeviceService>(),
        registry_.GetService<services::ILogger>());

    // Buffer service
    registry_.RegisterService<services::IBufferService, services::impl::BufferService>(
        registry_.GetService<services::IVulkanDeviceService>(),
        registry_.GetService<services::ILogger>());

    // Render command service
    registry_.RegisterService<services::IRenderCommandService, services::impl::RenderCommandService>(
        registry_.GetService<services::IVulkanDeviceService>(),
        registry_.GetService<services::ISwapchainService>(),
        registry_.GetService<services::ILogger>());

    // Graphics service (facade)
    registry_.RegisterService<services::IGraphicsService, services::impl::GraphicsService>(
        registry_.GetService<services::ILogger>(),
        registry_.GetService<services::IVulkanDeviceService>(),
        registry_.GetService<services::ISwapchainService>(),
        registry_.GetService<services::IPipelineService>(),
        registry_.GetService<services::IBufferService>(),
        registry_.GetService<services::IRenderCommandService>(),
        registry_.GetService<services::IWindowService>());

    // Script service
    registry_.RegisterService<services::IScriptService, services::impl::LuaScriptService>(
        scriptPath_, registry_.GetService<services::ILogger>());

    // Connect input service to script service for GUI input processing
    auto inputService = registry_.GetService<services::IInputService>();
    auto scriptService = registry_.GetService<services::IScriptService>();
    if (inputService && scriptService) {
        inputService->SetScriptService(scriptService.get());
    }

    // Scene service
    registry_.RegisterService<services::ISceneService, services::impl::SceneService>(
        registry_.GetService<services::IScriptService>(),
        registry_.GetService<services::ILogger>());

    // Audio service
    registry_.RegisterService<services::IAudioService, services::impl::SdlAudioService>(
        registry_.GetService<services::ILogger>());

    // GUI service
    registry_.RegisterService<services::IGuiService, services::impl::VulkanGuiService>(
        registry_.GetService<services::ILogger>());

    // Physics service
    registry_.RegisterService<services::IPhysicsService, services::impl::BulletPhysicsService>(
        registry_.GetService<services::ILogger>());

    logger_->Trace("ServiceBasedApp", "RegisterServices", "", "Exiting");
}

}  // namespace sdl3cpp::app