#pragma once

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_quadkey.hpp"
#include "services/interfaces/workflow/fs2024/data/vec/fs2024_vec_tile.hpp"

namespace sdl3cpp::fs2024 {

/// Where a vector point lies in its level-14 tile, as fractions of the
/// tile from its north-west corner (x east, y south; outside 0..1 in
/// the overlap margin). Measured, not documented anywhere: the u16
/// range is centred on the tile's centre with +y NORTH, and spans the
/// tile's own ground width plus a fixed 214.4 m on each side -- so the
/// unit is a little under 3 cm at London and grows with the tile toward
/// the equator. Points are clipped at 2686 and 62850, about 134 m past
/// the tile's edge, so neighbours overlap. Fitted against OpenStreetMap
/// at Westminster and against the overlap of neighbouring tiles at
/// London and Innsbruck (see fs2024_vec_real_test).
void VecPointInTile(const QuadTile& tile, const VecPoint& point, double& fx,
                    double& fy);

}  // namespace sdl3cpp::fs2024
