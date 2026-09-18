#include "services/interfaces/workflow/fs2024/landmark/fs2024_tile_landmarks.hpp"

#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_load.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"

#include <unordered_set>

namespace sdl3cpp::services::impl {
namespace {

/// The most binary glTF one landmark may load: FS2024's own LOD0 of the
/// Palace of Westminster is 3 MB, and a few city models run to tens.
constexpr std::size_t kLodBudgetBytes = 6u << 20;

}  // namespace

void EnsureFs2024LandmarkKits(
    SDL_GPUDevice* device, const Fs2024World& world,
    const std::vector<Fs2024LandmarkInstance>& instances,
    Fs2024LandmarkKits& kits) {
    for (const Fs2024LandmarkInstance& instance : instances) {
        if (kits.count(instance.entry.name)) continue;
        kits.emplace(instance.entry.name,
                     LoadFs2024LandmarkKit(device,
                                           world.paths.landmarkLibrary,
                                           instance.entry,
                                           world.paths.landmarkTextures,
                                           kLodBudgetBytes));
    }
}

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
        ReleaseFs2024LandmarkKitGpu(device, it->second);
        it = state.landmarkKits.erase(it);
    }
}

}  // namespace sdl3cpp::services::impl
