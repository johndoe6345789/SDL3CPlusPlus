#pragma once

#include <filesystem>
#include <string>

#include <rapidjson/document.h>

namespace sdl3cpp::services::impl::json_config {

class JsonConfigDocumentParser {
public:
    rapidjson::Document Parse(const std::filesystem::path& jsonPath,
                              const std::string& description) const;
};

}  // namespace sdl3cpp::services::impl::json_config
