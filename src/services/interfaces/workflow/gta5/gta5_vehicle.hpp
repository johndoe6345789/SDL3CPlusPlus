#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_spec.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// Read a vehicle from the map and put it on the road as a raycast
/// vehicle: a chassis body plus four sprung wheels, so it rides on
/// suspension and grips through corners rather than sliding as a box.
bool SpawnGta5Vehicle(Gta5StreamState& state, const Gta5VehicleSpec& spec,
                      const glm::vec3& position, float mass,
                      SDL_GPUDevice* device, btDiscreteDynamicsWorld* world,
                      const std::shared_ptr<ILogger>& logger);

/// Copy each chassis transform into the matrix its body draws with.
void UpdateGta5Vehicles(Gta5StreamState& state);

/// Apply engine, steering and brake to the seated car.
///
/// throttle and steer are -1..1; brake is 0..1. Steering eases toward
/// `steer` over `dt` seconds, and its lock narrows with speed.
void DriveGta5Vehicle(Gta5Vehicle& car, float throttle, float steer,
                      float brake, float dt);

/// What the pedals do: throttle -1..1 (back..forward), brake 0..1.
struct Gta5Pedals {
    float throttle{0.f};
    float brake{0.f};
};

/// From how hard the accelerator (RT, W) and the brake (LT, S) are
/// pressed, 0..1, as a driver means them: rolling forward the brake
/// brakes, progressively, and only at a standstill does it reverse;
/// rolling back the accelerator brakes first. The handbrake locks all
/// four.
Gta5Pedals DecideGta5Pedals(const Gta5Vehicle& car, float accelerate,
                            float slow, bool handbrake);

}  // namespace sdl3cpp::services::impl
