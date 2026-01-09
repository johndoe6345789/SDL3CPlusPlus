#include "json_config_document_parser.hpp"

#include <rapidjson/error/en.h>
#include <rapidjson/istreamwrapper.h>

#include <fstream>
#include <stdexcept>

namespace sdl3cpp::services::impl::json_config {

rapidjson::Document JsonConfigDocumentParser::Parse(const std::filesystem::path& jsonPath,
                                                    const std::string& description) const {
    std::ifstream configStream(jsonPath);
    if (!configStream) {
        throw std::runtime_error("Failed to open " + description + ": " + jsonPath.string());
    }

    rapidjson::IStreamWrapper inputWrapper(configStream);
    rapidjson::Document document;
    document.ParseStream(inputWrapper);
    if (document.HasParseError()) {
        throw std::runtime_error("Failed to parse " + description + " at " + jsonPath.string() +
                                 ": " + rapidjson::GetParseError_En(document.GetParseError()));
    }
    if (!document.IsObject()) {
        throw std::runtime_error("JSON " + description + " must contain an object at the root");
    }
    return document;
}

}  // namespace sdl3cpp::services::impl::json_config
