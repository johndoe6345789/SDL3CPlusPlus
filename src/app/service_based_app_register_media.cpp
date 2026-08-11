#include "app/service_based_app.hpp"
#include "services/impl/config/json_config_service.hpp"
#include "services/impl/config/config_compiler_service.hpp"
#include "services/impl/diagnostics/validation_tour_service.hpp"
#include "services/impl/render/render_graph_service.hpp"
#include "services/impl/scene/ecs_service.hpp"
#include "services/impl/shader/pipeline_compiler_service.hpp"
#include "services/impl/platform/sdl_window_service.hpp"
#include "services/impl/input/sdl_input_service.hpp"
#include "services/impl/audio/sdl_audio_service.hpp"
#include "services/impl/scene/mesh_service.hpp"
#include "events/i_event_bus.hpp"
#include "services/interfaces/i_platform_service.hpp"
#include "services/interfaces/i_probe_service.hpp"
#include "services/interfaces/i_render_graph_service.hpp"
#include "services/interfaces/i_validation_tour_service.hpp"

namespace sdl3cpp::app {

void ServiceBasedApp::RegisterMediaServices() {
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

    // Pipeline compiler service (shader compilation)
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

    // Mesh service
    registry_.RegisterService<services::IMeshService, services::impl::MeshService>(
        registry_.GetService<services::IConfigService>(),
        registry_.GetService<services::ILogger>());
}

}  // namespace sdl3cpp::app
