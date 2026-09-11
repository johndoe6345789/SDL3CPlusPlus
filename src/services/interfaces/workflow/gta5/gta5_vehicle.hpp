#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// Load a vehicle model and drop it into the world as a dynamic body.
///
/// Unlike map geometry this is not static: it has mass, so it falls onto
/// the road, settles on its suspension-less box, and can be shoved.
bool SpawnGta5Vehicle(Gta5StreamState& state, const std::string& modelPath,
                      const glm::vec3& position, float mass,
                      SDL_GPUDevice* device, btDiscreteDynamicsWorld* world,
                      const std::shared_ptr<ILogger>& logger);

/// Copy each vehicle's body transform into the matrix it draws with.
void UpdateGta5Vehicles(Gta5StreamState& state);

}  // namespace sdl3cpp::services::impl
