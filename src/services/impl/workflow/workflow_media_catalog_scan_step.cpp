#include "services/interfaces/workflow/workflow_media_catalog_scan_step.hpp"

#include "services/interfaces/config/json_config_document_parser.hpp"
#include "services/interfaces/workflow/media_catalog_builder.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowMediaCatalogScanStep::WorkflowMediaCatalogScanStep(
    std::shared_ptr<IConfigService> configService,
    std::shared_ptr<ILogger> logger)
    : configService_(std::move(configService)), logger_(std::move(logger)) {}

std::string WorkflowMediaCatalogScanStep::GetPluginId() const {
    return "media.catalog.scan";
}

void WorkflowMediaCatalogScanStep::Execute(const WorkflowStepDefinition& step,
                                            WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string outputKey =
        resolver.GetRequiredOutputKey(step, "catalog");

    // Get catalog config path parameter (required)
    auto catalogPathIt = step.parameters.find("catalog_config_path");
    if (catalogPathIt == step.parameters.end()) {
        throw std::runtime_error(
            "media.catalog.scan: missing required parameter "
            "'catalog_config_path'");
    }
    const std::string catalogPathParam = catalogPathIt->second.stringValue;

    // Get package root key parameter (optional, default "package.root")
    std::string packageRootKey = "package.root";
    auto packageRootKeyIt = step.parameters.find("package_root_key");
    if (packageRootKeyIt != step.parameters.end()) {
        packageRootKey = packageRootKeyIt->second.stringValue;
    }
    // Get package root from context
    const auto* packageRoot =
        context.TryGet<std::filesystem::path>(packageRootKey);
    if (!packageRoot || packageRoot->empty()) {
        throw std::runtime_error(
            "media.catalog.scan: package root not found in context at "
            "key '" + packageRootKey + "'");
    }

    if (!cachedCatalog_) {
        std::filesystem::path catalogPath = *packageRoot / catalogPathParam;
        cachedCatalog_ = LoadCatalog(catalogPath, *packageRoot);
        if (logger_) {
            std::size_t itemCount = 0;
            for (const auto& category : cachedCatalog_->categories) {
                itemCount += category.items.size();
            }
            logger_->Trace(
                "WorkflowMediaCatalogScanStep", "Execute",
                "categories=" +
                    std::to_string(cachedCatalog_->categories.size()) +
                    ", items=" + std::to_string(itemCount),
                "Catalog scanned");
        }
    }
    context.Set(outputKey, *cachedCatalog_);
}

MediaCatalog WorkflowMediaCatalogScanStep::LoadCatalog(
    const std::filesystem::path& catalogPath,
    const std::filesystem::path& packageRoot) const {
    json_config::JsonConfigDocumentParser parser;
    auto document = parser.Parse(catalogPath, "media catalog");
    return BuildMediaCatalog(document, packageRoot);
}

}  // namespace sdl3cpp::services::impl
