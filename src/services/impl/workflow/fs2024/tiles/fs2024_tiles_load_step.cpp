#include "services/interfaces/workflow/fs2024/tiles/fs2024_tiles_load_step.hpp"

#include "services/interfaces/workflow/fs2024/fs2024_step_params.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_load_pool.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_prepare.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"

#include <algorithm>
#include <chrono>
#include <thread>
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
    Fs2024TileStreamState& state = *state_;
    if (!device || !state.world) return;
    if (!state.pool) {
        const unsigned cores = std::thread::hardware_concurrency();
        Fs2024World* world = state.world.get();
        state.pool = std::make_shared<Fs2024LoadPool>(
            [world](const Fs2024TileKey& key) {
                return PrepareFs2024Tile(*world, key);
            },
            std::clamp(cores > 2 ? cores - 2 : 1u, 1u, 8u));
    }
    // Stop building what the viewer has moved on from, then queue the
    // rest, nearest first.
    for (const Fs2024TileKey& key : state.pool->Drop(
             [&](const Fs2024TileKey& k) { return state.wanted.count(k); })) {
        state.loading.erase(key);
    }
    for (const Fs2024TileKey& key : state.pendingLoad) {
        state.pool->Enqueue(key);
        state.loading.insert(key);
    }
    state.pendingLoad.clear();

    // Once at init: wait for every wanted tile, so the spawn point's
    // ground exists before the first frame.
    const bool force = Fs2024NumberOr(step, "force", 0.f) != 0.f;
    do {
        FinishFs2024ReadyTiles(state, device, physics,
                               force ? 1e9 : state.finishBudgetMs,
                               logger_.get(), stats_);
        if (force && !state.loading.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    } while (force && !state.loading.empty());
    ReportStats();
}

void WorkflowFs2024TilesLoadStep::ReportStats() {
    const auto now = std::chrono::steady_clock::now();
    if (!logger_ || now - lastReport_ < std::chrono::seconds(5)) return;
    lastReport_ = now;
    logger_->Info("fs2024.tiles: " + std::to_string(state_->resident.size()) +
                  " resident, " + std::to_string(state_->drawn.size()) +
                  " drawn, " + std::to_string(state_->loading.size()) +
                  " loading; finished " + std::to_string(stats_.finished) +
                  ", slowest " + std::to_string(stats_.worstMs) + " ms");
    stats_ = {};
}

}  // namespace sdl3cpp::services::impl
