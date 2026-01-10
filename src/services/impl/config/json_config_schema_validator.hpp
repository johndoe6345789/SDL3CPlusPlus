#pragma once

#include <filesystem>
#include <memory>

#include <rapidjson/document.h>

namespace sdl3cpp::services {
class ILogger;
class IProbeService;
}

namespace sdl3cpp::services::impl::json_config {

class JsonConfigSchemaValidator {
public:
    JsonConfigSchemaValidator(std::shared_ptr<ILogger> logger,
                              std::shared_ptr<IProbeService> probeService);

    void ValidateOrThrow(const rapidjson::Document& document,
                         const std::filesystem::path& configPath) const;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IProbeService> probeService_;
};

}  // namespace sdl3cpp::services::impl::json_config
