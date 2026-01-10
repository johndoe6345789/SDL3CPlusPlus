#include "json_config_document_loader.hpp"
#include "json_config_document_parser.hpp"
#include "json_config_extend_resolver.hpp"
#include "json_config_merge_service.hpp"
#include "services/interfaces/i_logger.hpp"

#include <stdexcept>
#include <string>
#include <system_error>

namespace sdl3cpp::services::impl::json_config {

JsonConfigDocumentLoader::JsonConfigDocumentLoader(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

rapidjson::Document JsonConfigDocumentLoader::Load(const std::filesystem::path& configPath) const {
    std::unordered_set<std::string> visited;
    return LoadRecursive(configPath, visited);
}

std::filesystem::path JsonConfigDocumentLoader::NormalizeConfigPath(const std::filesystem::path& path) const {
    std::error_code ec;
    auto canonicalPath = std::filesystem::weakly_canonical(path, ec);
    if (ec) {
        return std::filesystem::absolute(path);
    }
    return canonicalPath;
}

rapidjson::Document JsonConfigDocumentLoader::LoadRecursive(const std::filesystem::path& configPath,
                                                            std::unordered_set<std::string>& visited) const {
    const auto normalizedPath = NormalizeConfigPath(configPath);
    const std::string pathKey = normalizedPath.string();
    if (!visited.insert(pathKey).second) {
        throw std::runtime_error("Config extends cycle detected at " + pathKey);
    }

    if (logger_) {
        logger_->Trace("JsonConfigService", "LoadConfigDocumentRecursive",
                       "configPath=" + pathKey, "Loading config document");
    }

    JsonConfigDocumentParser parser;
    rapidjson::Document document = parser.Parse(normalizedPath, "config file");

    JsonConfigExtendResolver extendResolver;
    auto extendPaths = extendResolver.Extract(document, normalizedPath);
    if (document.HasMember("extends")) {
        document.RemoveMember("extends");
    }

    if (extendPaths.empty()) {
        visited.erase(pathKey);
        return document;
    }

    if (logger_) {
        logger_->Trace("JsonConfigService", "LoadConfigDocumentRecursive",
                       "configPath=" + pathKey + ", extendsCount=" + std::to_string(extendPaths.size()),
                       "Merging extended configs");
    }

    JsonConfigMergeService mergeService(logger_);
    rapidjson::Document merged;
    merged.SetObject();
    auto& allocator = merged.GetAllocator();
    for (const auto& extendPath : extendPaths) {
        auto baseDoc = LoadRecursive(extendPath, visited);
        mergeService.Merge(merged, baseDoc, allocator, extendPath.string());
    }
    mergeService.Merge(merged, document, allocator, normalizedPath.string());

    visited.erase(pathKey);
    return merged;
}

}  // namespace sdl3cpp::services::impl::json_config
