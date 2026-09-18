#include "services/interfaces/workflow/fs2024/assemble/fs2024_tile_building_mesh.hpp"

#include "services/interfaces/workflow/fs2024/building/fs2024_building_mesh.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

float GroundUnder(const std::vector<Point2>& footprint,
                  const Fs2024Heightfield& field) {
    float lowest = 1e30f;
    for (const Point2& p : footprint) {
        lowest = std::min(lowest, Fs2024HeightAt(field, p.x, p.y));
    }
    return lowest;
}

void Raise(std::vector<BspRenderVertex>& vertices, std::size_t from,
           float by) {
    for (std::size_t i = from; i < vertices.size(); ++i) vertices[i].y += by;
}

}  // namespace

Fs2024BuildingMeshCpu MeshFs2024TileBuildings(
    const std::vector<Fs2024BuildingPlan>& plans,
    const Fs2024Heightfield& field) {
    Fs2024BuildingMeshCpu mesh;
    for (const Fs2024BuildingPlan& plan : plans) {
        const std::size_t walls = mesh.wallVertices.size();
        const std::size_t roofs = mesh.roofVertices.size();
        AppendBuildingMesh(plan.footprint, plan.height, plan.roof,
                           plan.roofRise, mesh.wallVertices,
                           mesh.wallIndices, mesh.roofVertices,
                           mesh.roofIndices);
        const float ground = GroundUnder(plan.footprint, field);
        Raise(mesh.wallVertices, walls, ground);
        Raise(mesh.roofVertices, roofs, ground);
    }
    return mesh;
}

}  // namespace sdl3cpp::services::impl
