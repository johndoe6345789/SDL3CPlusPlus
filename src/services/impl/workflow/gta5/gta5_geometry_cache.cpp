#include "services/interfaces/workflow/gta5/gta5_geometry_cache.hpp"

#include "services/interfaces/workflow/gta5/gta5_collision_shape.hpp"
#include "services/interfaces/workflow/gta5/gta5_geometry_request.hpp"
#include "services/interfaces/workflow/gta5/gta5_geometry_upload.hpp"

namespace sdl3cpp::services::impl {

Gta5Geometry* GetOrLoadGta5Geometry(Gta5StreamState& state,
                                    const Gta5Placement& placement,
                                    SDL_GPUDevice* device,
                                    const std::shared_ptr<ILogger>& logger,
                                    bool* pending) {
    // From a binary ymap: prepared by the load pool, off this thread.
    if (placement.archetypeHash != 0 && state.pool) {
        bool wait = false;
        Gta5Geometry* geometry =
            RequestGta5IndexedGeometry(state, placement, wait);
        if (pending) *pending = wait;
        return geometry;
    }

    // From a legacy tile file: its glTF, imported here.
    if (placement.modelPath.empty() || !device) {
        if (state.reportedMissing.insert(placement.archetype).second &&
            logger) {
            logger->Warn("gta5.tiles.load: archetype '" +
                         placement.archetype +
                         "' has no model to load; its placements are skipped");
        }
        return nullptr;
    }
    const auto cached = state.geometryCache.find(placement.archetype);
    if (cached != state.geometryCache.end()) {
        return cached->second.usable ? &cached->second : nullptr;
    }
    // Inserted before building so a failure is remembered as an unusable
    // entry, rather than retried for every instance every frame.
    Gta5Geometry& geometry = state.geometryCache[placement.archetype];
    if (!BuildGta5Geometry(placement, device, state.arena, state.uploads,
                           state.textureCache, geometry, logger)) {
        return nullptr;
    }
    return &geometry;
}

void SweepGta5GeometryCache(Gta5StreamState& state, SDL_GPUDevice* device,
                            const std::shared_ptr<ILogger>& logger) {
    if (!device) return;
    int released = 0;
    for (auto it = state.geometryCache.begin();
         it != state.geometryCache.end();) {
        Gta5Geometry& geometry = it->second;
        if (!geometry.usable || geometry.references > 0) {
            ++it;
            continue;
        }
        for (const Gta5SubMesh& sub : geometry.subMeshes) {
            state.arena.Free(sub.slot);
        }
        // Textures are left alone: they are shared far more widely than
        // one archetype, and the cache outlives any single district.
        ReleaseGta5CollisionShape(geometry);
        it = state.geometryCache.erase(it);
        ++released;
    }
    if (released > 0 && logger) {
        logger->Trace("SweepGta5GeometryCache", "Execute",
                      "released=" + std::to_string(released),
                      "freed unreferenced archetype buffers");
    }
}

}  // namespace sdl3cpp::services::impl
