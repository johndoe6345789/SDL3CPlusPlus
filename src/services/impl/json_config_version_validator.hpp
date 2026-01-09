#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include <rapidjson/document.h>

namespace sdl3cpp::services {
class ILogger;
}

namespace sdl3cpp::services::impl::json_config {

class JsonConfigVersionValidator {
public:
    explicit JsonConfigVersionValidator(std::shared_ptr<ILogger> logger);

    std::optional<int> Validate(const rapidjson::Value& document,
                                const std::filesystem::path& configPath) const;

private:
    std::optional<int> ReadVersionField(const rapidjson::Value& document,
                                        const char* fieldName,
                                        const std::filesystem::path& configPath) const;

    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl::json_config
