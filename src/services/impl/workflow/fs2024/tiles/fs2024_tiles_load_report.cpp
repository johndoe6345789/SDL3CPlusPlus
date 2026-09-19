#include "services/interfaces/workflow/fs2024/tiles/fs2024_tiles_load_step.hpp"

#include <chrono>

namespace sdl3cpp::services::impl {

void WorkflowFs2024TilesLoadStep::ReportStats() {
    const auto now = std::chrono::steady_clock::now();
    if (!logger_ || now - lastReport_ < std::chrono::seconds(5)) return;
    lastReport_ = now;
    std::size_t vegChunks = 0;
    for (const auto& [key, tile] : state_->resident) {
        vegChunks += tile.vegetationChunks.size();
    }
    const std::size_t vegSpecies = state_->vegetationSpecies.size();
    logger_->Info("fs2024.tiles: " + std::to_string(state_->resident.size()) +
                  " resident, " + std::to_string(state_->drawn.size()) +
                  " drawn, " + std::to_string(state_->loading.size()) +
                  " loading; finished " + std::to_string(stats_.finished) +
                  ", slowest " + std::to_string(stats_.worstMs) + " ms; " +
                  std::to_string(vegChunks) + " veg chunks, " +
                  std::to_string(vegSpecies) + " species");
    stats_ = {};
}

}  // namespace sdl3cpp::services::impl
