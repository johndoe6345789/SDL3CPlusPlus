#pragma once

#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"
#include "services/interfaces/workflow_context.hpp"

namespace sdl3cpp::services::impl {

/// Nearest car within `reach` metres of a point, or -1.
int FindGta5VehicleNear(const Gta5StreamState& state, const btVector3& point,
                        float reach);

/// Pin the player body to the car it is sitting in.
void RideGta5Vehicle(const Gta5Vehicle& car, btRigidBody* player,
                     WorkflowContext& context);

/// Put the player back on the road beside the car.
void LeaveGta5Vehicle(const Gta5Vehicle& car, btRigidBody* player);

}  // namespace sdl3cpp::services::impl
