#pragma once

#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_instance.hpp"
#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_kit_gpu.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_heightfield.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_key.hpp"

#include <SDL3/SDL_gpu.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services::impl {

struct Fs2024World;
struct Fs2024TileStreamState;

/// Every uploaded landmark model, by its own GXML name.
using Fs2024LandmarkKits =
    std::unordered_map<std::string, Fs2024LandmarkKitGpu>;

/// The landmarks FS2024 stands inside tile `key` -- at any level, every
/// finest-level quad inside it -- in the tile's own space: each where
/// the game puts it, on the ground of `field` (still tile-local) under
/// its own origin.
std::vector<Fs2024LandmarkInstance> PlaceFs2024TileLandmarks(
    const Fs2024World& world, const Fs2024TileKey& key,
    const Fs2024Heightfield& field);

/// Releases every kit no resident tile still stands, so flying away
/// from a city gives its landmarks back.
void ReleaseUnusedFs2024LandmarkKits(SDL_GPUDevice* device,
                                     Fs2024TileStreamState& state);

}  // namespace sdl3cpp::services::impl
