#include "services/interfaces/workflow/fs2024/landmark/fs2024_tile_landmarks.hpp"

#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_load.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"

#include <unordered_set>

namespace sdl3cpp::services::impl {

void ReleaseUnusedFs2024LandmarkKits(SDL_GPUDevice* device,
                                     Fs2024TileStreamState& state) {
    std::unordered_set<std::string> used;
    for (const auto& [key, tile] : state.resident) {
        for (const auto& instance : tile.landmarks) {
            used.insert(instance.entry.name);
        }
    }
    for (auto it = state.landmarkKits.begin();
         it != state.landmarkKits.end();) {
        if (used.count(it->first)) {
            ++it;
            continue;
        }
        if (state.world) state.world->landmarkBook.SetOnGpu(it->first, false);
        ReleaseFs2024LandmarkKitGpu(device, it->second);
        it = state.landmarkKits.erase(it);
    }
}

}  // namespace sdl3cpp::services::impl
