#pragma once

#include <filesystem>
#include <vector>

#include <rapidjson/document.h>

namespace sdl3cpp::services::impl::json_config {

class JsonConfigExtendResolver {
public:
    std::vector<std::filesystem::path> Extract(const rapidjson::Value& document,
                                               const std::filesystem::path& configPath) const;
};

}  // namespace sdl3cpp::services::impl::json_config
