#pragma once

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

// ─────────────────────────────────────────────────────────────────────
// Q3Trace
//   Result of sweeping the player's box through the world.
//   Split from q3_pm_types.hpp so the brush tracer can produce one
//   without pulling in Bullet.
// ─────────────────────────────────────────────────────────────────────
struct Q3Trace {
    bool      hit{false};
    float     fraction{1.f};   // 0 = started solid, 1 = no hit
    glm::vec3 endPos{0.f};
    glm::vec3 normal{0.f, 1.f, 0.f};
    bool      startSolid{false};
};

}  // namespace sdl3cpp::services::impl
