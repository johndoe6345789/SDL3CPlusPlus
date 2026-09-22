#include "services/interfaces/workflow/stunts/world/stunts_track_mesh.hpp"

#include "services/interfaces/workflow/stunts/world/stunts_mesh_build.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr std::uint8_t kEdges[4] = {kStuntsLinkNorth, kStuntsLinkEast,
                                    kStuntsLinkSouth, kStuntsLinkWest};

void AppendGroundCell(GeometryPlaneMesh& mesh, const glm::vec3& centre,
                      float tileSize) {
    const float half = tileSize * 0.5f;
    AppendStuntsQuad(mesh, centre + glm::vec3(-half, 0.f, -half),
                     centre + glm::vec3(half, 0.f, -half),
                     centre + glm::vec3(half, 0.f, half),
                     centre + glm::vec3(-half, 0.f, half), 1.f);
}

void AppendRoadTile(GeometryPlaneMesh& mesh, const StuntsTile& tile,
                    const glm::vec3& centre, const StuntsMeshParams& params) {
    if (tile.kind == StuntsTileKind::Corner) {
        AppendStuntsCorner(mesh, centre, tile.links, params.tileSize,
                           params.roadWidth, params.cornerSegments);
        return;
    }
    for (const std::uint8_t edge : kEdges) {
        if (tile.links & edge) {
            AppendStuntsArm(mesh, centre, edge, params.tileSize,
                            params.roadWidth);
        }
    }
}

}  // namespace

glm::vec3 StuntsCellCentre(int x, int y, float tileSize) {
    const float span = static_cast<float>(kStuntsGrid) * tileSize;
    return {static_cast<float>(x) * tileSize - span * 0.5f + tileSize * 0.5f,
            0.f,
            static_cast<float>(y) * tileSize - span * 0.5f + tileSize * 0.5f};
}

StuntsTrackMesh BuildStuntsTrackMesh(const StuntsTrack& track,
                                     const StuntsTileTable& table,
                                     const StuntsMeshParams& params) {
    StuntsTrackMesh mesh;
    if (!track.loaded) return mesh;

    for (int y = 0; y < kStuntsGrid; ++y) {
        for (int x = 0; x < kStuntsGrid; ++x) {
            const glm::vec3 centre = StuntsCellCentre(x, y, params.tileSize);
            AppendGroundCell(mesh.ground, centre, params.tileSize);

            const StuntsTile& tile =
                table.tiles[StuntsRoadAt(track, x, y)];
            if (tile.kind == StuntsTileKind::None) continue;
            const glm::vec3 surface =
                centre + glm::vec3(0.f, params.roadHeight, 0.f);
            AppendRoadTile(mesh.road, tile, surface, params);
            ++mesh.roadTiles;
        }
    }
    return mesh;
}

}  // namespace sdl3cpp::services::impl
