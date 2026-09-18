#pragma once

#include "services/interfaces/workflow/fs2024/terrain/fs2024_vegetation_kit_gpu.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_prepared_tile.hpp"

#include <string>
#include <unordered_map>

namespace sdl3cpp::services::impl {

struct Fs2024TileStreamState;

using Fs2024VegSpeciesTextures =
    std::unordered_map<std::string, Fs2024VegSpeciesGpu>;

/// Reads and uploads one species' own albedo texture array as a BC7
/// sampler2DArray, its layers passed through compressed.
Fs2024VegSpeciesGpu UploadFs2024VegetationSpecies(SDL_GPUDevice* device,
                                                  const std::string& path);

/// Puts on the GPU every species this tile stands that `species` does
/// not hold yet, and uploads its own merged mesh per species.
std::vector<Fs2024VegetationChunkGpu> FinishFs2024TileVegetation(
    SDL_GPUDevice* device, const Fs2024PreparedTile& prepared,
    Fs2024VegSpeciesTextures& species);

/// Releases every species texture no resident tile stands any more.
void ReleaseUnusedFs2024Vegetation(SDL_GPUDevice* device,
                                   Fs2024TileStreamState& state);

}  // namespace sdl3cpp::services::impl
