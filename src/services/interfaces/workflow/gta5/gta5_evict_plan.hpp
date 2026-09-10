#pragma once

#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <string>
#include <unordered_set>
#include <vector>

namespace sdl3cpp::services::impl {

struct Gta5EvictPlan {
    /// Scene-object tags whose objects must go.
    std::unordered_set<std::string> purge;
    /// Tiles that left the evict radius and are forgotten entirely.
    std::vector<Gta5TileCoord> dropped;
    /// Tiles kept but reset so load rebuilds them at a new detail band.
    int rebuilt{0};
};

/// Decide what eviction should remove this frame, and apply the resident
/// side of it: dropped tiles are erased and rebuilt tiles are reset.
///
/// The caller is left to filter the scene object list against `purge`.
Gta5EvictPlan ApplyGta5EvictPlan(Gta5StreamState& state,
                                 const std::string& objectTypePrefix);

}  // namespace sdl3cpp::services::impl
