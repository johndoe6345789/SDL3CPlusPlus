#include "app/service_based_app.hpp"
#include "services/impl/scene/physics_bridge_service.hpp"
#include "services/impl/shader/shader_system_registry.hpp"
#include "services/impl/graphics/graphics_service.hpp"
#include "services/impl/graphics/graphics_backend_factory.hpp"
#include "services/impl/scene/scene_service.hpp"
#include "services/impl/scene/bullet_physics_service.hpp"
#include "services/impl/render/render_coordinator_service.hpp"
#include "services/interfaces/workflow/frame/frame_workflow_service.hpp"
#include "services/impl/app/application_loop_service.hpp"
#include "events/i_event_bus.hpp"
#include "services/interfaces/i_platform_service.hpp"
#include "services/interfaces/i_probe_service.hpp"
#include "services/interfaces/i_shader_system_registry.hpp"
#include "services/interfaces/i_validation_tour_service.hpp"
#include "services/interfaces/i_frame_workflow_service.hpp"

namespace sdl3cpp::app {

void ServiceBasedApp::RegisterGraphicsServices(const std::string& backendName) {
    // Physics bridge services
    registry_.RegisterService<services::IPhysicsBridgeService, services::impl::PhysicsBridgeService>(
        registry_.GetService<services::ILogger>());

    // Shader system registry (pluggable shader system selection)
    registry_.RegisterService<services::IShaderSystemRegistry, services::impl::ShaderSystemRegistry>(
        registry_.GetService<services::IConfigService>(),
        registry_.GetService<services::IConfigCompilerService>(),
        registry_.GetService<services::ILogger>(),
        gamePackage_);

    logger_->Info("ServiceBasedApp::RegisterServices: Creating graphics backend: " + backendName);

    // Create graphics backend using factory (supports SDL3 GPU, GXM, etc.)
    auto graphicsBackend = services::impl::GraphicsBackendFactory::CreateFromWorkflow(
        backendName,
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

    // Physics service
    registry_.RegisterService<services::IPhysicsService, services::impl::BulletPhysicsService>(
        registry_.GetService<services::ILogger>());

    // Render coordinator service
    registry_.RegisterService<services::IRenderCoordinatorService, services::impl::RenderCoordinatorService>(
        registry_.GetService<services::ILogger>(),
        registry_.GetService<services::IConfigCompilerService>(),
        registry_.GetService<services::IGraphicsService>(),
        registry_.GetService<services::IShaderSystemRegistry>(),
        registry_.GetService<services::ISceneService>(),
        registry_.GetService<services::IValidationTourService>());

    // Frame workflow service (registered after all dependencies: physics, scene, render coordinator)
    registry_.RegisterService<services::IFrameWorkflowService, services::impl::FrameWorkflowService>(
        registry_.GetService<services::ILogger>(),
        registry_.GetService<services::IConfigService>(),
        registry_.GetService<services::IAudioService>(),
        registry_.GetService<services::IGraphicsService>(),
        registry_.GetService<services::IInputService>(),
        registry_.GetService<services::IMeshService>(),
        registry_.GetService<services::IPhysicsService>(),
        registry_.GetService<services::ISceneService>(),
        registry_.GetService<services::IRenderCoordinatorService>(),
        registry_.GetService<services::IValidationTourService>(),
        registry_.GetService<services::IWindowService>(),
        registry_.GetService<services::IPlatformService>(),
        registry_.GetService<services::IShaderSystemRegistry>(),
        gamePackage_);

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
}

}  // namespace sdl3cpp::app
