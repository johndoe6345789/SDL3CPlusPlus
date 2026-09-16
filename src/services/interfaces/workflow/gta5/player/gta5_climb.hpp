#pragma once

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Something in front of the player they can climb onto.
struct Gta5Ledge {
    glm::vec3 top{0.f};  // where the feet end up, on its upper surface
    float rise{0.f};     // metres above the feet they started on
};

/// Looks ahead of @p feet along @p forward (level) for a wall at knee
/// height with a top 0.4-2 m up, and room above that top for a body
/// @p height tall. The player's own body, @p self, never counts.
bool FindGta5Ledge(btDiscreteDynamicsWorld* world,
                   const btCollisionObject* self, const glm::vec3& feet,
                   const glm::vec3& forward, float height, Gta5Ledge& out);

/// A climb under way: up first, then over, as GTA's vault reads.
struct Gta5Climb {
    bool active{false};
    glm::vec3 from{0.f};  // the body's origin when it began
    glm::vec3 to{0.f};    // and where it ends
    float t{0.f};         // seconds in
    float duration{0.f};
};

/// The body's origin @p climb.t seconds in.
glm::vec3 Gta5ClimbAt(const Gta5Climb& climb);

}  // namespace sdl3cpp::services::impl
