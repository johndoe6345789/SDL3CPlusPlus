#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_shift.hpp"

#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_release.hpp"

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::services::impl {
namespace {

void MoveBody(btDiscreteDynamicsWorld* physics, btRigidBody* body,
              const glm::vec2& shift) {
    if (!body) return;
    btTransform transform = body->getWorldTransform();
    transform.setOrigin(transform.getOrigin() -
                        btVector3(shift.x, 0.f, shift.y));
    body->setWorldTransform(transform);
    if (body->getMotionState()) body->getMotionState()->setWorldTransform(
        transform);
    if (physics) physics->updateSingleAabb(body);
}

}  // namespace

void ShiftFs2024Tiles(Fs2024TileStreamState& state,
                      btDiscreteDynamicsWorld* physics,
                      const Fs2024Rebase& rebase) {
    StopFs2024Loads(state);
    std::unordered_map<Fs2024TileKey, Fs2024LoadedTile> moved;
    for (auto& [key, tile] : state.resident) {
        const int shift = kFs2024FinestLevel - key.level;
        const Fs2024TileKey renamed{key.x - (rebase.tilesX >> shift),
                                    key.z - (rebase.tilesY >> shift),
                                    key.level};
        tile.offset -= glm::vec3(rebase.shift.x, 0.f, rebase.shift.y);
        tile.terrain.field.origin -= rebase.shift;
        MoveBody(physics, tile.terrain.collision.body, rebase.shift);
        moved.emplace(renamed, std::move(tile));
    }
    state.resident = std::move(moved);
    state.drawn.clear();
    state.wanted.clear();
    state.pendingLoad.clear();
    state.pendingEvict.clear();
    state.missing.clear();
}

}  // namespace sdl3cpp::services::impl
