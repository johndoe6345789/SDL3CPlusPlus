#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_spec.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_types.hpp"

#include <SDL3/SDL_gpu.h>

#include <memory>

namespace sdl3cpp::services::impl {

/// Read a vehicle's body from the map, turn it to face +z, and upload it
/// -- with collision, since the chassis box is sized from it -- as
/// car.instance.geometry, cached under "vehicle:<model>". Its axles come
/// from the same .yft's wheel bones into `setup`; without them the wheels
/// fall back to a layout from the box.
bool LoadGta5VehicleChassis(Gta5StreamState& state,
                            const Gta5VehicleSpec& spec,
                            SDL_GPUDevice* device, Gta5Vehicle& car,
                            Gta5WheelSetup& setup,
                            const std::shared_ptr<ILogger>& logger);

/// Read the pack wheel once and upload it twice, sized from `spec`: as
/// modelled for the right-hand pair, mirrored for the left. Each wheel
/// takes its side's geometry. Sets car.hasWheels on success.
bool LoadGta5VehicleWheelMeshes(Gta5StreamState& state,
                                const Gta5VehicleSpec& spec,
                                SDL_GPUDevice* device, Gta5Vehicle& car,
                                const Gta5WheelSetup& setup,
                                const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
