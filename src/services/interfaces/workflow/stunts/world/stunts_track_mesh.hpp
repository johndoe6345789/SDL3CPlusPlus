#pragma once

#include "services/interfaces/workflow/geometry/geometry_plane_helpers.hpp"
#include "services/interfaces/workflow/stunts/data/stunts_tile_table.hpp"
#include "services/interfaces/workflow/stunts/data/stunts_track.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// How the fixed 30x30 grid is laid out in engine metres.
struct StuntsMeshParams {
    float tileSize = 24.f;    ///< Edge of one grid cell.
    float roadWidth = 8.f;    ///< Driveable width of a road piece.
    float roadHeight = 0.05f; ///< Lift over the ground, to stop z-fighting.
    int cornerSegments = 8;   ///< Chords per quarter turn.
};

/// The two meshes a track draws: its road surface and its ground.
struct StuntsTrackMesh {
    GeometryPlaneMesh road;
    GeometryPlaneMesh ground;
    int roadTiles = 0;
};

/// Centre of cell (x, y) in engine space, with the grid centred on the
/// origin and +Z running south, matching the file's row order.
glm::vec3 StuntsCellCentre(int x, int y, float tileSize);

/// Builds both meshes from a parsed track and its tile table.
StuntsTrackMesh BuildStuntsTrackMesh(const StuntsTrack& track,
                                     const StuntsTileTable& table,
                                     const StuntsMeshParams& params);

}  // namespace sdl3cpp::services::impl
