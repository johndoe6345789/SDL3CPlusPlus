#include "services/interfaces/workflow/gta5/stream/gta5_geometry_request.hpp"

namespace sdl3cpp::services::impl {

Gta5Geometry* RequestGta5IndexedGeometry(Gta5StreamState& state,
                                         const Gta5Placement& placement,
                                         bool& pending) {
    pending = false;
    if (!state.pool) return nullptr;
    Gta5Geometry& geometry = state.geometryCache[placement.archetype];
    if (geometry.usable) return &geometry;
    if (geometry.failed) return nullptr;
    pending = true;
    if (!geometry.pending) {
        geometry.pending = true;
        state.pool->Enqueue(placement.archetype, placement.archetypeHash);
    }
    return nullptr;
}

void PrefetchGta5Tile(Gta5StreamState& state, Gta5ResidentTile& resident) {
    if (resident.prefetched || !state.pool) return;
    resident.prefetched = true;
    bool pending = false;
    for (const Gta5Placement& placement : resident.placements) {
        // The band test spawning applies, so nothing is read that this
        // tile will not draw.
        if (placement.archetypeHash == 0 ||
            static_cast<int>(placement.lod) <
                static_cast<int>(resident.bandAtSpawn)) {
            continue;
        }
        RequestGta5IndexedGeometry(state, placement, pending);
    }
}

}  // namespace sdl3cpp::services::impl
