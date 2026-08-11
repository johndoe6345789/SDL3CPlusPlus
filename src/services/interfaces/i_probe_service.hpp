#pragma once

#include <memory>
#include <string>

namespace sdl3cpp::services {

/**
 * Probe service interface for diagnostic and configuration operations.
 *
 * This service provides mechanisms for:
 * - Configuration schema probing
 * - Configuration migration
 * - Diagnostic data collection
 */
class IProbeService {
public:
    virtual ~IProbeService() = default;

    /**
     * Get a unique identifier for this service.
     */
    virtual std::string GetServiceId() const = 0;
};

}  // namespace sdl3cpp::services
