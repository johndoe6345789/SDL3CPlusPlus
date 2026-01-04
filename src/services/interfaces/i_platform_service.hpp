#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace sdl3cpp::services {

class IPlatformService {
public:
    virtual ~IPlatformService() = default;

    virtual std::optional<std::filesystem::path> GetUserConfigDirectory() const = 0;
    virtual std::string GetPlatformError() const = 0;
};

}  // namespace sdl3cpp::services
