#pragma once

#include "../interfaces/i_application_loop_service.hpp"
#include "../interfaces/i_audio_service.hpp"
#include "../interfaces/i_input_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../interfaces/i_physics_service.hpp"
#include "../interfaces/i_scene_service.hpp"
#include "../interfaces/i_window_service.hpp"
#include "../../events/i_event_bus.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

class ApplicationLoopService : public IApplicationLoopService {
public:
    ApplicationLoopService(std::shared_ptr<ILogger> logger,
                           std::shared_ptr<IWindowService> windowService,
                           std::shared_ptr<events::IEventBus> eventBus,
                           std::shared_ptr<IInputService> inputService,
                           std::shared_ptr<IPhysicsService> physicsService,
                           std::shared_ptr<ISceneService> sceneService,
                           std::shared_ptr<IAudioService> audioService);
    ~ApplicationLoopService() override = default;

    void Run() override;

private:
    void HandleEvents();
    void ProcessFrame(float deltaTime);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IWindowService> windowService_;
    std::shared_ptr<events::IEventBus> eventBus_;
    std::shared_ptr<IInputService> inputService_;
    std::shared_ptr<IPhysicsService> physicsService_;
    std::shared_ptr<ISceneService> sceneService_;
    std::shared_ptr<IAudioService> audioService_;
    bool running_ = false;
};

}  // namespace sdl3cpp::services::impl
