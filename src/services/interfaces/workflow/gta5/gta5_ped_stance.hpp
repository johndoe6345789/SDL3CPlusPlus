#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Which way the ped faces and how far up its hands are, eased towards
/// where it should be this frame.
struct Gta5Stance {
    float yaw{3.14159265f};
    float aim{0.f};
};

/// Armed, it squares up to whatever the camera looks at; otherwise it
/// faces the way it walks. `run` is its ground velocity, `dt` seconds.
void SettleGta5Stance(Gta5Stance& stance, WorkflowContext& context,
                      const glm::vec2& run, float dt);

}  // namespace sdl3cpp::services::impl
