#include "services/interfaces/workflow/fs2024/tiles/fs2024_tiles_load_step.hpp"

#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_step_params.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_world_tile_load.hpp"

#include <algorithm>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowFs2024TilesLoadStep::WorkflowFs2024TilesLoadStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<Fs2024TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowFs2024TilesLoadStep::GetPluginId() const {
    return "fs2024.tiles.load";
}

void WorkflowFs2024TilesLoadStep::Execute(const WorkflowStepDefinition& step,
                                          WorkflowContext& context) {
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* physics =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    if (!device || !state_->world || state_->pendingLoad.empty()) return;

    const bool force = Fs2024NumberOr(step, "force", 0.f) != 0.f;
    const std::size_t budget =
        force ? state_->pendingLoad.size()
              : std::min<std::size_t>(state_->maxLoadsPerCall,
                                      state_->pendingLoad.size());
    for (std::size_t i = 0; i < budget; ++i) {
        const Fs2024TileKey key = state_->pendingLoad[i];
        try {
            state_->resident.emplace(
                key, LoadFs2024WorldTile(device, physics, *state_->world, key));
        } catch (const std::exception& error) {
            state_->missing.insert(key);
            if (logger_) {
                logger_->Warn("fs2024.tiles.load: tile (" +
                              std::to_string(key.x) + ", " +
                              std::to_string(key.z) + ") failed: " +
                              error.what());
            }
        }
    }
    state_->pendingLoad.erase(state_->pendingLoad.begin(),
                              state_->pendingLoad.begin() +
                                  static_cast<long>(budget));
}

}  // namespace sdl3cpp::services::impl
