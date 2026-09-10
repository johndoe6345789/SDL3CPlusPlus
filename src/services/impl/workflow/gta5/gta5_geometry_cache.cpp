#include "services/interfaces/workflow/gta5/gta5_geometry_cache.hpp"

#include "services/interfaces/workflow/gta5/gta5_mesh_import.hpp"

namespace sdl3cpp::services::impl {
namespace {

void ReportMissingModel(Gta5StreamState& state,
                        const Gta5Placement& placement,
                        const std::shared_ptr<ILogger>& logger) {
    if (state.reportedMissing.insert(placement.archetype).second && logger) {
        logger->Warn("gta5.tiles.load: archetype '" + placement.archetype +
                     "' has no exported model; its placements are skipped");
    }
}

}  // namespace

const Gta5Geometry* GetOrLoadGta5Geometry(
    Gta5StreamState& state, const Gta5Placement& placement,
    const std::shared_ptr<ILogger>& logger) {
    if (placement.modelPath.empty()) {
        ReportMissingModel(state, placement, logger);
        return nullptr;
    }

    const auto cached = state.geometryCache.find(placement.archetype);
    if (cached != state.geometryCache.end()) {
        return cached->second.usable ? &cached->second : nullptr;
    }

    // Inserted before the import so that a failure is remembered as an
    // unusable entry, rather than retried for every instance every frame.
    Gta5Geometry& geometry = state.geometryCache[placement.archetype];
    geometry.usable = false;

    if (!ImportGta5Mesh(placement.modelPath, placement.archetype, geometry,
                        logger)) {
        return nullptr;
    }
    return &geometry;
}

}  // namespace sdl3cpp::services::impl
