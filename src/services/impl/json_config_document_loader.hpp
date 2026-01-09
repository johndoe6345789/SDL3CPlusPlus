#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_set>

#include <rapidjson/document.h>

namespace sdl3cpp::services {
class ILogger;
}

namespace sdl3cpp::services::impl::json_config {

class JsonConfigDocumentLoader {
public:
    explicit JsonConfigDocumentLoader(std::shared_ptr<ILogger> logger);

    rapidjson::Document Load(const std::filesystem::path& configPath) const;

private:
    rapidjson::Document LoadRecursive(const std::filesystem::path& configPath,
                                      std::unordered_set<std::string>& visited) const;
    std::filesystem::path NormalizeConfigPath(const std::filesystem::path& path) const;

    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl::json_config
