#pragma once

#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"
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

/// The seated car driven from the keys and the pad (gta5.pad.*): pedals
/// as DecideGta5Pedals, steering from A and D or the stick. Returns its
/// speed along its heading, m/s, published as gta5.car.speed.
float ControlGta5Vehicle(Gta5Vehicle& car, WorkflowContext& context,
                         float dt);

/// Take car `index` out of the world and free it; the cars after it
/// move up one. Its meshes stay cached.
/// Everything one car holds: its bodies out of the world, its shapes
/// and its hold on the meshes. Leaves the struct empty and safe to drop.
void DestroyGta5Vehicle(Gta5Vehicle& car, btDiscreteDynamicsWorld* world);

void RemoveGta5Vehicle(Gta5StreamState& state, std::size_t index,
                       btDiscreteDynamicsWorld* world);

/// A respray: its vehicle_paint parts take `paint`.
void RepaintGta5Vehicle(Gta5Vehicle& car, const glm::vec3& paint);

/// Put the car down at `at`: upright on its heading -- or facing `yaw`
/// about +y, 0 being +z, its forward -- and at rest.
void MoveGta5Vehicle(Gta5Vehicle& car, const glm::vec3& at);
void MoveGta5Vehicle(Gta5Vehicle& car, const glm::vec3& at, float yaw);

}  // namespace sdl3cpp::services::impl
