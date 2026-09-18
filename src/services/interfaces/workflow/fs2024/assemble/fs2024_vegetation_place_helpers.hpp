#pragma once

#include "services/interfaces/workflow/fs2024/assemble/fs2024_vegetation_build.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

/// Tile spacing, metres, for a real per-hectare density -- see
/// fs2024_vegetation_build.cpp for why this is a fixed budget rather
/// than FS2024's own (much denser) real spacing.
float Fs2024VegSpacingFor(float instancesPerHectare);

/// The land class at tile-local (x, z), from a `classSize` square
/// raster (row 0 north), the same lookup fs2024_sea_build.cpp uses.
int Fs2024VegClassAt(const std::vector<std::uint8_t>& classes, int classSize,
                     float x, float z, float tileSize);

/// A jittered spot inside grid cell (c, r) of a `spacing`-metre grid,
/// accepted only when it falls on `landClass`'s own ground.
bool Fs2024VegSpot(int landClass, const std::vector<std::uint8_t>& classes,
                   int classSize, float tileSize, float spacing,
                   std::uint64_t seed, int r, int c, float& x, float& z);

/// Picks a species and variation for grid cell (c, r) of `rule`'s own
/// mix (weighted, deterministic from `seed`), and appends its billboard
/// at (x, z) to that species' own group. Does nothing when the rule's
/// species do not resolve to any real texture.
void Fs2024VegPlaceOne(const sdl3cpp::fs2024::VegetationLibrary& library,
                       const sdl3cpp::fs2024::VegBiomeRule& rule,
                       int landClass, float x, float z,
                       const Fs2024Heightfield& field, std::uint64_t seed,
                       int r, int c,
                       std::vector<Fs2024VegetationGroupCpu>& groups);

/// This species' own group of the tile's mesh, made empty on first use.
Fs2024VegetationGroupCpu& Fs2024VegGroupFor(
    std::vector<Fs2024VegetationGroupCpu>& groups,
    const sdl3cpp::fs2024::VegSpecies& species, const std::string& albedo);

/// One weighted pick among `items`, deterministic from `roll` (0..1):
/// the first for which `roll * totalWeight` falls in its own share.
template <typename T, typename Weight>
const T* Fs2024VegPickWeighted(const std::vector<T>& items, Weight weight,
                               float roll) {
    float total = 0.f;
    for (const T& item : items) total += std::max(weight(item), 0.f);
    if (total <= 0.f) return items.empty() ? nullptr : &items.front();
    float at = roll * total;
    for (const T& item : items) {
        at -= std::max(weight(item), 0.f);
        if (at <= 0.f) return &item;
    }
    return &items.back();
}

}  // namespace sdl3cpp::services::impl
