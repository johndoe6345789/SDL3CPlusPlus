#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Which way the ped faces and how far up its hands are, eased towards
/// where it should be this frame.
struct Gta5Stance {
    float yaw{3.14159265f};
    /// How far the weapon is up, 0 hanging to 1 at the sights, eased at
    /// both ends: what the arms are posed by.
    float aim{0.f};
    /// The raise itself, running at a set pace so it takes a known time
    /// rather than creeping in. `aim` is this, shaped.
    float raise{0.f};
};

/// Armed, it squares up to whatever the camera looks at; otherwise it
/// faces the way it walks. `run` is its ground velocity, `dt` seconds.
/// `base` is the heading the ped's own bind pose already carries, from
/// Gta5PedBaseYaw: the turn is counted from there, so the body faces
/// the same way its arms reach.
void SettleGta5Stance(Gta5Stance& stance, WorkflowContext& context,
                      const glm::vec2& run, float dt, float base);

}  // namespace sdl3cpp::services::impl
