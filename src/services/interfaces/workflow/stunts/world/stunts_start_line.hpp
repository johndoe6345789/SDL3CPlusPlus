#pragma once

#include "services/interfaces/workflow/stunts/data/stunts_tile_table.hpp"
#include "services/interfaces/workflow/stunts/data/stunts_track.hpp"
#include "services/interfaces/workflow/stunts/world/stunts_track_mesh.hpp"

namespace sdl3cpp::services::impl {

struct StuntsStartLine;

/**
 * @brief Picks where a lap begins on a parsed track.
 *
 * Tracks carry no explicit start marker, so this takes the first
 * straight in reading order -- the north-west-most one, as the game's
 * own grid order runs -- and faces the car along it, which puts the
 * car on the road and pointing down it on every stock track.
 */
StuntsStartLine FindStuntsStartLine(const StuntsTrack& track,
                                    const StuntsTileTable& table,
                                    const StuntsMeshParams& params);

}  // namespace sdl3cpp::services::impl
