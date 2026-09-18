#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_finish.hpp"

#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_load.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"

namespace sdl3cpp::services::impl {
namespace {

void Adopt(SDL_GPUDevice* device, Fs2024World& world,
           const Fs2024LandmarkMesh& mesh, Fs2024LandmarkKits& kits) {
    if (kits.count(mesh.name)) return;
    kits.emplace(mesh.name, UploadFs2024LandmarkKit(device, mesh));
    world.landmarkBook.SetOnGpu(mesh.name, true);
}

}  // namespace

void AdoptFs2024TileKits(SDL_GPUDevice* device, Fs2024World& world,
                         const Fs2024PreparedTile& prepared,
                         Fs2024LandmarkKits& kits) {
    for (const auto& mesh : prepared.models) Adopt(device, world, *mesh, kits);
    for (const Fs2024LandmarkInstance& instance : prepared.landmarks) {
        if (kits.count(instance.entry.name)) continue;
        Adopt(device, world,
              ReadFs2024LandmarkMesh(world.paths.landmarkLibrary,
                                     instance.entry,
                                     world.paths.landmarkTextures,
                                     kFs2024LandmarkLodBudget),
              kits);
    }
}

}  // namespace sdl3cpp::services::impl
