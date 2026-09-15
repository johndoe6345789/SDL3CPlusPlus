#pragma once

#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_types.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Every crossing near `at` that is not already known, added.
void Gta5FindJunctions(Gta5Traffic& traffic, const Gta5Roads& roads,
                       const glm::vec3& at);

/// Junctions near `at` brought up to date, far ones dropped, and every
/// clock advanced. Call once a frame before the cars are driven.
void StepGta5Lights(Gta5Traffic& traffic, const Gta5Roads& roads,
                    const glm::vec3& at, float dt);

/// Whether a car arriving at `node` along `heading` may cross. Nodes
/// with no light -- anything that is not a junction -- are always open.
bool Gta5LightOpen(const Gta5Traffic& traffic, std::uint32_t node,
                   float heading);

/// Work every car's pedals and wheel for a frame: on towards the node
/// it is making for, easing off for the car in front and holding at
/// the line for a light that is against it.
void DriveGta5Traffic(Gta5Traffic& traffic, const Gta5Roads& roads,
                      float dt);

/// Keep the road around `at` populated, and let go of what is behind.
/// Cars are built and destroyed here, so this owns the physics bodies.
void KeepGta5Traffic(Gta5Traffic& traffic, const Gta5Roads& roads,
                     Gta5StreamState& state, const glm::vec3& at,
                     SDL_GPUDevice* device, btDiscreteDynamicsWorld* world,
                     const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
