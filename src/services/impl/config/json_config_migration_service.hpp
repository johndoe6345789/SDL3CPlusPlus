#pragma once

#include <filesystem>
#include <memory>

#include <rapidjson/document.h>

namespace sdl3cpp::services {
class ILogger;
class IProbeService;
}

namespace sdl3cpp::services::impl::json_config {

class JsonConfigMigrationService {
public:
    JsonConfigMigrationService(std::shared_ptr<ILogger> logger,
                               std::shared_ptr<IProbeService> probeService);

    bool Apply(rapidjson::Document& document,
               int fromVersion,
               int toVersion,
               const std::filesystem::path& configPath) const;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IProbeService> probeService_;
};

}  // namespace sdl3cpp::services::impl::json_config
