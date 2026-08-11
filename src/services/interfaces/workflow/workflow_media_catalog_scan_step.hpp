#pragma once

#include "services/interfaces/i_config_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/media_types.hpp"

#include <memory>
#include <optional>

namespace sdl3cpp::services::impl {

/**
 * Generic step: Scan and load a media catalog from JSON configuration
 *
 * Parametric, reusable step that loads ANY media catalog (audio, video, images, etc.)
 * from a JSON file with structure:
 * {
 *   "categories": [
 *     {
 *       "id": "category_id",
 *       "name": "Category Name",
 *       "path": "path/to/directory"
 *     }
 *   ]
 * }
 *
 * Parameters:
 *   catalog_config_path: Path to JSON catalog file (relative to package root)
 *   package_root_key: Context key where package root is stored (default: "package.root")
 *
 * Output:
 *   output_key: Context key where MediaCatalog is stored
 */
class WorkflowMediaCatalogScanStep final : public IWorkflowStep {
public:
    WorkflowMediaCatalogScanStep(std::shared_ptr<IConfigService> configService,
                                 std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    MediaCatalog LoadCatalog(const std::filesystem::path& catalogPath,
                             const std::filesystem::path& packageRoot) const;

    std::shared_ptr<IConfigService> configService_;
    std::shared_ptr<ILogger> logger_;
    std::optional<MediaCatalog> cachedCatalog_;
};

}  // namespace sdl3cpp::services::impl
