#pragma once

#include <rapidjson/document.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace json_config {

class JsonConfigDocumentParser {
public:
    rapidjson::Document Parse(const std::filesystem::path& path, const std::string& description) {
        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + path.string());
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string jsonStr = buffer.str();

        rapidjson::Document doc;
        if (doc.Parse(jsonStr.c_str()).HasParseError()) {
            throw std::runtime_error("Failed to parse JSON in " + description + " at " + path.string());
        }

        return doc;
    }
};

}  // namespace json_config
