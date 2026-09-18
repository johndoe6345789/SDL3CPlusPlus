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

/// The roof's own colour rides in the (otherwise unused) lightmap uv:
/// 15 bits of 5-bit channels in lm_u -- exact in a float -- and 1 in
/// lm_v to say there is one. fs2024_terrain.frag unpacks it.
/// A flat roof no data set coloured is bitumen grey, not the pitched
/// roof's clay tile (a stand-in: FS2024 itself does not say).
void Tint(std::vector<BspRenderVertex>& vertices, std::size_t from,
          const Fs2024BuildingPlan& plan) {
    const bool flat = plan.roof == RoofShape::Flat;
    if (!plan.hasRoofColour && !flat) return;
    const int grey = 12;
    const float packed = static_cast<float>(
        plan.hasRoofColour
            ? plan.roofRed | plan.roofGreen << 5 | plan.roofBlue << 10
            : grey | grey << 5 | grey << 10);
    for (std::size_t i = from; i < vertices.size(); ++i) {
        vertices[i].lm_u = packed;
        vertices[i].lm_v = 1.f;
    }
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
        Tint(mesh.roofVertices, roofs, plan);
    }
    return mesh;
}

}  // namespace sdl3cpp::services::impl
