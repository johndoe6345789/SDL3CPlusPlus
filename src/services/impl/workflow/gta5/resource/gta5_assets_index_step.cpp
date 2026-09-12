#include "services/interfaces/workflow/gta5/resource/gta5_assets_index_step.hpp"

#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_world_config_load.hpp"

#include <filesystem>
#include <future>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5AssetsIndexStep::WorkflowGta5AssetsIndexStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5AssetsIndexStep::GetPluginId() const {
    return "gta5.assets.index";
}

void WorkflowGta5AssetsIndexStep::Execute(const WorkflowStepDefinition& step,
                                          WorkflowContext& context) {
    if (!state_ || state_->assets || state_->assetsPending.valid()) return;
    // Inflated dictionaries and drawables kept for reuse; 4 GB by default.
    state_->resources.budget = static_cast<std::uint64_t>(
        Gta5NumberOr(step, "resource_cache_mb", 8192.f)) << 20;

    const std::string root = Gta5ParameterOr(step, "map_dir", "");
    std::error_code ec;
    if (root.empty() || !std::filesystem::is_directory(root, ec)) {
        if (logger_) {
            logger_->Warn("gta5.assets.index: map_dir '" + root +
                          "' is not a folder; tiles come from assets/tiles");
        }
        return;
    }

    // The index is built before the frame loop has read the world config,
    // and its tiles must be the configured grid's, so it is read here.
    Gta5WorldConfig world;
    LoadGta5WorldConfig(
        Gta5ResolvePath(step, context, "config",
                        "packages/gta5/config/gta5_world.json"),
        world, logger_);

    state_->assetsPending = std::async(std::launch::async, [root, world] {
        return std::make_shared<const Gta5AssetIndex>(
            BuildGta5AssetIndex(root, world));
    });
    if (logger_) {
        logger_->Info("gta5.assets.index: indexing '" + root +
                      "' in the background");
    }
}

}  // namespace sdl3cpp::services::impl
