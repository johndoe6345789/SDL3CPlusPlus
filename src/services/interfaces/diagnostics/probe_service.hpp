#pragma once

#include "services/interfaces/i_probe_service.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Default implementation of the probe service.
 *
 * Provides diagnostic and configuration probing capabilities.
 */
class ProbeService final : public IProbeService {
public:
    explicit ProbeService(std::shared_ptr<ILogger> logger = nullptr);

    std::string GetServiceId() const override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
