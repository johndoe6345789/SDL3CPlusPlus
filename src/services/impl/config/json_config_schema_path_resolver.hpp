#pragma once

#include <filesystem>

namespace sdl3cpp::services::impl::json_config {

class JsonConfigSchemaPathResolver {
public:
    std::filesystem::path Resolve(const std::filesystem::path& configPath) const;
};

}  // namespace sdl3cpp::services::impl::json_config
