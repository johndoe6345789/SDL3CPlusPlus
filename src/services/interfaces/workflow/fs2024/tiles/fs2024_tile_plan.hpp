#pragma once

#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_key.hpp"

#include <unordered_set>
#include <vector>

namespace sdl3cpp::services::impl {

/// What streaming does next, given what it wants and what it has.
struct Fs2024TilePlan {
    std::vector<Fs2024TileKey> load;          ///< in the wanted order
    std::unordered_set<Fs2024TileKey> draw;   ///< never overlapping
    std::vector<Fs2024TileKey> evict;
};

/// Plans a change of detail without ever leaving a hole or drawing two
/// tiles over one patch of ground. A wanted tile not yet `resident` (or
/// known `empty`) is loaded, and meanwhile the ground it covers is drawn
/// from what is resident: its nearest resident ancestor if it has one
/// -- which then hides every finer tile inside it until all of them are
/// ready -- or else its resident descendants. Resident tiles neither
/// wanted nor drawn are evicted, so an old tile goes only once its
/// replacement can be drawn.
Fs2024TilePlan PlanFs2024Tiles(
    const std::vector<Fs2024TileKey>& wanted,
    const std::unordered_set<Fs2024TileKey>& resident,
    const std::unordered_set<Fs2024TileKey>& empty);

}  // namespace sdl3cpp::services::impl
