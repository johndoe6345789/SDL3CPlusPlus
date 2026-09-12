#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Put the player at `at`, at rest, in both the rigid body and q3.ps.
/// The movement state keeps its own velocity, which would otherwise build
/// up while pinned and be released all at once.
void PinGta5Player(WorkflowContext& context, btRigidBody* body,
                   const glm::vec3& at);

/// Pin the player a metre above the first surface under `at`, so a spawn
/// set high to clear unknown terrain lets go on the ground rather than
/// dropping the whole way. False when there is nothing below.
bool DropGta5PlayerToGround(WorkflowContext& context, btRigidBody* body,
                            const glm::vec3& at);

}  // namespace sdl3cpp::services::impl
