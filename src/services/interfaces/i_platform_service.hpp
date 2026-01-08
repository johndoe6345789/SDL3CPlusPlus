#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace sdl3cpp::services {

class IPlatformService {
public:
    virtual ~IPlatformService() = default;

    virtual std::optional<std::filesystem::path> GetUserConfigDirectory() const = 0;
    virtual std::string GetPlatformError() const = 0;
    virtual std::string GetPlatformName() const = 0;
    virtual std::string GetCurrentVideoDriverName() const = 0;
    virtual std::vector<std::string> GetAvailableVideoDrivers() const = 0;
    virtual std::vector<std::string> GetAvailableRenderDrivers() const = 0;
    virtual void LogSystemInfo() const = 0;
};

}  // namespace sdl3cpp::services
