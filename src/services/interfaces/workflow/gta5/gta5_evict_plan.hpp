#pragma once

#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

namespace sdl3cpp::services::impl {

struct Gta5EvictResult {
    /// Tiles that left the evict radius and were forgotten entirely.
    int dropped{0};
    /// Tiles kept but reset, so load rebuilds them at a new detail band.
    int rebuilt{0};
    /// Instances released between the two.
    int instancesReleased{0};
};

/// Drop tiles outside the evict radius and reset the ones gta5.lod.select
/// flagged, releasing each instance's hold on its archetype geometry.
///
/// Geometry buffers are not freed here: SweepGta5GeometryCache does that
/// once, after the references have been given up.
Gta5EvictResult ApplyGta5EvictPlan(Gta5StreamState& state);

}  // namespace sdl3cpp::services::impl
