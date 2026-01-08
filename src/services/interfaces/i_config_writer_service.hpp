#pragma once

#include "config_types.hpp"
#include <filesystem>

namespace sdl3cpp::services {

/**
 * @brief Configuration writer service interface.
 */
class IConfigWriterService {
public:
    virtual ~IConfigWriterService() = default;
    virtual void WriteConfig(const RuntimeConfig& config, const std::filesystem::path& configPath) = 0;
};

}  // namespace sdl3cpp::services
