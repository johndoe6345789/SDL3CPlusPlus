#pragma once

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::services::impl {

/// Where to draw a body: its motion state, which Bullet interpolates
/// between fixed physics steps, rather than its world transform, which
/// only moves when a step runs. Physics steps at 60 Hz and frames come
/// at 240, so drawn from the world transform a fast car holds still for
/// three frames and jumps on the fourth -- a vibration, and a camera
/// following it shimmers the whole view. Falls back to the world
/// transform for a body without a motion state.
btTransform Gta5ShownTransform(const btRigidBody* body);

}  // namespace sdl3cpp::services::impl
