#include "services/interfaces/workflow/workflow_media_catalog_scan_step.hpp"

#include "services/interfaces/config/json_config_document_parser.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string PrettyItemName(const std::string& fileName) {
    std::string base = fileName;
    const auto dot = base.find_last_of('.');
    if (dot != std::string::npos) {
        base = base.substr(0, dot);
    }
    for (char& ch : base) {
        if (ch == '_' || ch == '-') {
            ch = ' ';
        }
    }
    bool capitalize = true;
    for (char& ch : base) {
        if (std::isspace(static_cast<unsigned char>(ch))) {
            capitalize = true;
        } else if (capitalize) {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            capitalize = false;
        } else {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
    }
    return base;
}

std::vector<MediaItem> LoadItems(const std::filesystem::path& directory) {
    std::vector<MediaItem> items;
    if (!std::filesystem::exists(directory)) {
        return items;
    }
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const std::string fileName = entry.path().filename().string();
        MediaItem item{};
        item.id = fileName;
        item.label = PrettyItemName(fileName);
        item.path = entry.path();
        items.push_back(std::move(item));
    }
    std::sort(items.begin(), items.end(),
              [](const MediaItem& a, const MediaItem& b) {
                  return ToLower(a.id) < ToLower(b.id);
              });
    return items;
}

MediaCatalog BuildCatalog(const rapidjson::Document& document,
                          const std::filesystem::path& packageRoot) {
    if (!document.HasMember("categories") || !document["categories"].IsArray()) {
        throw std::runtime_error("media catalog requires a categories array");
    }
    MediaCatalog catalog{};
    catalog.catalogRoot = packageRoot;
    const auto& categories = document["categories"];
    catalog.categories.reserve(categories.Size());
    for (const auto& categoryValue : categories.GetArray()) {
        if (!categoryValue.IsObject()) {
            throw std::runtime_error("media catalog categories must be objects");
        }
        if (!categoryValue.HasMember("id") || !categoryValue["id"].IsString()) {
            throw std::runtime_error("media catalog category requires string id");
        }
        if (!categoryValue.HasMember("name") || !categoryValue["name"].IsString()) {
            throw std::runtime_error("media catalog category requires string name");
        }
        if (!categoryValue.HasMember("path") || !categoryValue["path"].IsString()) {
            throw std::runtime_error("media catalog category requires string path");
        }
        MediaCategory category{};
        category.id = categoryValue["id"].GetString();
        category.name = categoryValue["name"].GetString();
        category.basePath = packageRoot / categoryValue["path"].GetString();
        category.items = LoadItems(category.basePath);
        catalog.categories.push_back(std::move(category));
    }
    return catalog;
}

}  // namespace

WorkflowMediaCatalogScanStep::WorkflowMediaCatalogScanStep(
    std::shared_ptr<IConfigService> configService,
    std::shared_ptr<ILogger> logger)
    : configService_(std::move(configService)),
      logger_(std::move(logger)) {}

std::string WorkflowMediaCatalogScanStep::GetPluginId() const {
    return "media.catalog.scan";
}

void WorkflowMediaCatalogScanStep::Execute(const WorkflowStepDefinition& step,
                                            WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "catalog");

    // Get catalog config path parameter (required)
    auto catalogPathIt = step.parameters.find("catalog_config_path");
    if (catalogPathIt == step.parameters.end()) {
        throw std::runtime_error("media.catalog.scan: missing required parameter 'catalog_config_path'");
    }
    const std::string catalogPathParam = catalogPathIt->second.stringValue;

    // Get package root key parameter (optional, default "package.root")
    std::string packageRootKey = "package.root";
    auto packageRootKeyIt = step.parameters.find("package_root_key");
    if (packageRootKeyIt != step.parameters.end()) {
        packageRootKey = packageRootKeyIt->second.stringValue;
    }

    // Get package root from context
    const auto* packageRoot = context.TryGet<std::filesystem::path>(packageRootKey);
    if (!packageRoot || packageRoot->empty()) {
        throw std::runtime_error("media.catalog.scan: package root not found in context at key '" + packageRootKey + "'");
    }

    if (!cachedCatalog_) {
        std::filesystem::path catalogPath = *packageRoot / catalogPathParam;
        cachedCatalog_ = LoadCatalog(catalogPath, *packageRoot);
        if (logger_) {
            std::size_t itemCount = 0;
            for (const auto& category : cachedCatalog_->categories) {
                itemCount += category.items.size();
            }
            logger_->Trace("WorkflowMediaCatalogScanStep", "Execute",
                           "categories=" + std::to_string(cachedCatalog_->categories.size()) +
                               ", items=" + std::to_string(itemCount),
                           "Catalog scanned");
        }
    }

    context.Set(outputKey, *cachedCatalog_);
}

MediaCatalog WorkflowMediaCatalogScanStep::LoadCatalog(const std::filesystem::path& catalogPath,
                                                        const std::filesystem::path& packageRoot) const {
    json_config::JsonConfigDocumentParser parser;
    auto document = parser.Parse(catalogPath, "media catalog");
    return BuildCatalog(document, packageRoot);
}

}  // namespace sdl3cpp::services::impl
