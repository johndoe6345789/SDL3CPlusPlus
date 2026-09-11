#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <SDL3/SDL_gpu.h>

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// Load the four wheel meshes the converter wrote beside a chassis.
///
/// Looks for "<model>_wheel0..3.gltf". Leaves car.hasWheels false when
/// they are absent, in which case the car still drives -- the wheels are
/// simply baked into the body and do not turn.
void LoadGta5VehicleWheels(Gta5StreamState& state, Gta5Vehicle& car,
                           const std::string& modelPath,
                           SDL_GPUDevice* device, Gta5WheelSetup& setup,
                           const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
