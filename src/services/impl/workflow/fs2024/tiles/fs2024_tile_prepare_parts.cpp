#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_prepare.hpp"

#include "services/interfaces/workflow/fs2024/assemble/fs2024_tile_building_mesh.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"

namespace sdl3cpp::services::impl {

Fs2024LandmarkBoundsMap PrepareFs2024TileLandmarks(Fs2024World& world,
                                                   Fs2024PreparedTile& tile) {
    Fs2024LandmarkBoundsMap bounds;
    for (const Fs2024LandmarkInstance& instance : tile.landmarks) {
        const std::string& name = instance.entry.name;
        if (bounds.count(name)) continue;
        if (const auto uploaded = world.landmarkBook.Uploaded(name)) {
            bounds.emplace(name, *uploaded);
            continue;
        }
        auto mesh = std::make_shared<const Fs2024LandmarkMesh>(
            ReadFs2024LandmarkMesh(world.paths.landmarkLibrary,
                                   instance.entry,
                                   world.paths.landmarkTextures,
                                   kFs2024LandmarkLodBudget));
        world.landmarkBook.Record(name, mesh->bounds);
        bounds.emplace(name, mesh->bounds);
        tile.models.push_back(std::move(mesh));
    }
    return bounds;
}

Fs2024BuildingMeshCpu PrepareFs2024TileBuildings(
    Fs2024World& world, const Fs2024PreparedTile& tile,
    const Fs2024LandmarkBoundsMap& bounds) {
    const float tileSize = world.origin.TileSize();
    int quadX = 0, quadY = 0;
    Fs2024QuadOfKey(world.origin, tile.key, quadX, quadY);
    const int side = 1 << (kFs2024FinestLevel - tile.key.level);
    std::vector<Fs2024BuildingPlan> plans;
    for (int dy = 0; dy < side; ++dy) {
        for (int dx = 0; dx < side; ++dx) {
            auto quad = PlanFs2024TileBuildings(
                world.buildings->ReadTile(sdl3cpp::fs2024::QuadTile{
                    quadX * side + dx, quadY * side + dy,
                    kFs2024FinestLevel}),
                tileSize);
            for (Fs2024BuildingPlan& plan : quad) {
                for (Point2& point : plan.footprint) {
                    point.x += static_cast<float>(dx) * tileSize;
                    point.y += static_cast<float>(dy) * tileSize;
                }
                plans.push_back(std::move(plan));
            }
        }
    }
    DropFs2024BuildingsUnderLandmarks(plans, tile.landmarks, bounds);
    return MeshFs2024TileBuildings(plans, tile.field);
}

}  // namespace sdl3cpp::services::impl
