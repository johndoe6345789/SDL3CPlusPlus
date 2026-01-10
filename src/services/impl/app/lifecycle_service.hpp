#pragma once

#include "services/interfaces/i_lifecycle_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "../../../di/service_registry.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

class LifecycleService : public ILifecycleService {
public:
    LifecycleService(di::ServiceRegistry& registry, std::shared_ptr<ILogger> logger);
    ~LifecycleService() override = default;

    void InitializeAll() override;
    void ShutdownAll() noexcept override;

private:
    di::ServiceRegistry& registry_;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
