#include "services/interfaces/workflow/gta5/gta5_prepared_finish.hpp"

#include "services/interfaces/workflow/gta5/gta5_collision_shape.hpp"
#include "services/interfaces/workflow/gta5/gta5_drawable_geometry.hpp"
#include "services/interfaces/workflow/gta5/gta5_indexed_texture.hpp"

#include <SDL3/SDL_timer.h>

namespace sdl3cpp::services::impl {
namespace {

bool TexturesIn(const Gta5StreamState& state, const Gta5PreparedGeometry& p) {
    for (const std::uint32_t texture : p.textures) {
        if (!state.textureCache.count(Gta5TextureKey(texture))) return false;
    }
    return true;
}

void Complete(Gta5StreamState& state, Gta5PreparedGeometry& prepared,
              SDL_GPUDevice* device, const std::shared_ptr<ILogger>& logger) {
    Gta5Geometry& geometry = state.geometryCache[prepared.key];
    geometry.pending = false;
    if (prepared.mesh.parts.empty() ||
        !UploadGta5MeshGeometry(state, prepared.mesh, device, geometry, false,
                                prepared.key, logger)) {
        geometry.failed = true;
        ReleaseGta5CollisionShape(prepared.collision);
        if (state.reportedMissing.insert(prepared.key).second && logger) {
            logger->Warn("gta5.tiles.load: archetype '" + prepared.key +
                         "' has no drawable to load; its placements are "
                         "skipped");
        }
        return;
    }
    // Built on the worker. Moving an array keeps its buffer, which is
    // what the BVH points into.
    Gta5Geometry& built = prepared.collision;
    geometry.collisionVertices = std::move(built.collisionVertices);
    geometry.collisionIndices = std::move(built.collisionIndices);
    geometry.collisionMesh = built.collisionMesh;
    geometry.collisionShape = built.collisionShape;
    built.collisionMesh = nullptr;
    built.collisionShape = nullptr;
}

}  // namespace

void FinishGta5PreparedGeometry(Gta5StreamState& state, SDL_GPUDevice* device,
                                float budgetMs,
                                const std::shared_ptr<ILogger>& logger) {
    if (!state.pool || !device) return;
    const std::uint64_t start = SDL_GetTicksNS();
    const auto spent = [&] {
        return static_cast<float>(SDL_GetTicksNS() - start) / 1e6f >= budgetMs;
    };
    while (!spent()) {
        std::vector<Gta5PreparedGeometry> batch = state.pool->Take(8);
        if (batch.empty()) break;
        for (Gta5PreparedGeometry& prepared : batch) {
            for (const Gta5TextureBlob& blob : prepared.blobs) {
                InstallGta5TextureBlob(state, blob, device);
            }
            prepared.blobs.clear();
            state.waiting.push_back(std::move(prepared));
        }
    }
    for (std::size_t i = 0; i < state.waiting.size() && !spent();) {
        if (!TexturesIn(state, state.waiting[i])) {
            ++i;
            continue;
        }
        Complete(state, state.waiting[i], device, logger);
        state.waiting.erase(state.waiting.begin() +
                            static_cast<std::ptrdiff_t>(i));
    }
}

}  // namespace sdl3cpp::services::impl
