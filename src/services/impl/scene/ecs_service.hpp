#pragma once

#include "services/interfaces/i_ecs_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "../../../di/lifecycle.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

class EcsService : public IEcsService,
                   public di::IShutdownable {
public:
    explicit EcsService(std::shared_ptr<ILogger> logger);
    ~EcsService() override = default;

    entt::registry& GetRegistry() override { return registry_; }
    const entt::registry& GetRegistry() const override { return registry_; }

    void Shutdown() noexcept override;

private:
    entt::registry registry_;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
