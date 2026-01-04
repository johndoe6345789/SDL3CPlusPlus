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
#include <stdexcept>

namespace sdl3cpp::app {

ServiceBasedApp::ServiceBasedApp(const std::filesystem::path& scriptPath)
    : scriptPath_(scriptPath) {
    logging::TraceGuard trace("ServiceBasedApp::ServiceBasedApp");

    try {
        SetupSDL();
        RegisterServices();

        lifecycleController_ = std::make_unique<controllers::LifecycleController>(registry_);
        applicationController_ = std::make_unique<controllers::ApplicationController>(registry_);
    } catch (const std::exception& e) {
        logging::Logger::GetInstance().Error("Failed to initialize ServiceBasedApp: " + std::string(e.what()));
        throw;
    }
}

ServiceBasedApp::~ServiceBasedApp() {
    logging::TraceGuard trace("ServiceBasedApp::~ServiceBasedApp");

    applicationController_.reset();
    lifecycleController_.reset();
}

void ServiceBasedApp::Run() {
    logging::TraceGuard trace("ServiceBasedApp::Run");

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

        // Run the main application loop
        applicationController_->Run();

        // Shutdown all services
        lifecycleController_->ShutdownAll();

    } catch (const std::exception& e) {
        logging::Logger::GetInstance().Error("Application error: " + std::string(e.what()));
        throw;
    }
}

void ServiceBasedApp::SetupSDL() {
    logging::TraceGuard trace("ServiceBasedApp::SetupSDL");

    // SDL initialization is handled by the window service
    // Don't initialize SDL here to avoid double initialization
}

void ServiceBasedApp::RegisterServices() {
    logging::TraceGuard trace("ServiceBasedApp::RegisterServices");

    // Event bus (needed by window service)
    auto eventBus = std::make_shared<events::EventBus>();
    registry_.RegisterService<events::EventBus>(eventBus);

    // Configuration service
    auto configService = std::make_shared<services::impl::JsonConfigService>();
    registry_.RegisterService<services::IConfigService>(configService);

    // Window service
    auto windowService = std::make_shared<services::impl::SdlWindowService>(eventBus);
    registry_.RegisterService<services::IWindowService>(windowService);

    // Input service
    auto inputService = std::make_shared<services::impl::SdlInputService>();
    registry_.RegisterService<services::IInputService>(inputService);

    // Vulkan device service
    auto deviceService = std::make_shared<services::impl::VulkanDeviceService>();
    registry_.RegisterService<services::IVulkanDeviceService>(deviceService);

    // Swapchain service
    auto swapchainService = std::make_shared<services::impl::SwapchainService>(deviceService);
    registry_.RegisterService<services::ISwapchainService>(swapchainService);

    // Pipeline service
    auto pipelineService = std::make_shared<services::impl::PipelineService>(deviceService);
    registry_.RegisterService<services::IPipelineService>(pipelineService);

    // Buffer service
    auto bufferService = std::make_shared<services::impl::BufferService>(deviceService);
    registry_.RegisterService<services::IBufferService>(bufferService);

    // Render command service
    auto renderCommandService = std::make_shared<services::impl::RenderCommandService>(
        deviceService, swapchainService, pipelineService, bufferService);
    registry_.RegisterService<services::IRenderCommandService>(renderCommandService);

    // Graphics service (facade)
    auto graphicsService = std::make_shared<services::impl::GraphicsService>(
        deviceService, swapchainService, pipelineService, bufferService, renderCommandService);
    registry_.RegisterService<services::IGraphicsService>(graphicsService);

    // Script service
    auto scriptService = std::make_shared<services::impl::LuaScriptService>(scriptPath_);
    registry_.RegisterService<services::IScriptService>(scriptService);

    // Scene service
    auto sceneService = std::make_shared<services::impl::SceneService>(scriptService);
    registry_.RegisterService<services::ISceneService>(sceneService);

    // Audio service
    auto audioService = std::make_shared<services::impl::SdlAudioService>();
    registry_.RegisterService<services::IAudioService>(audioService);

    // GUI service
    auto guiService = std::make_shared<services::impl::VulkanGuiService>();
    registry_.RegisterService<services::IGuiService>(guiService);

    // Physics service
    auto physicsService = std::make_shared<services::impl::BulletPhysicsService>();
    registry_.RegisterService<services::IPhysicsService>(physicsService);
}

}  // namespace sdl3cpp::app