#pragma once

#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/// Puts FS2024's ground material array on the GPU untouched (BC1 sRGB,
/// every layer and mip) and fills the world's per-class material table
/// for `climate` (FS2024's variant 1-4; 2 is temperate).
void UploadFs2024GroundMaterials(SDL_GPUDevice* device, Fs2024World& world,
                                 int climate);

/// Bakes the building kit from FS2024's generator data and publishes it
/// under the keys fs2024.terrain.draw reads (`fs2024_building_gpu`,
/// `fs2024_building_sampler`, `fs2024_roof_gpu`, `fs2024_roof_sampler`).
void PublishFs2024BuildingKit(SDL_GPUDevice* device, const Fs2024World& world,
                              WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
