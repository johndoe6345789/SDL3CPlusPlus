#pragma once

#include <string>
#include <vector>

namespace sdl3cpp::fs2024 {

/// One species named in a biome rule's own mix.
struct VegBiomeSpecies {
    std::string name;
    float spawnRatio = 1.f;
};

/// One of FS2024's own biome rules (fs-base/vegetation/*.xml): how many
/// trees a hectare of it holds, and which species make it up. FS2024
/// picks a rule by matching its Potential Natural Vegetation or eco
/// region rasters; this engine picks by name instead (see
/// fs2024_vegetation_pick.hpp), so only the rule's own real density and
/// species mix are borrowed, not its raster-matching machinery.
struct VegBiomeRule {
    std::string name;
    float instancesPerHectare = 0.f;
    std::vector<VegBiomeSpecies> species;
};

/// Parses one of FS2024's own biome rule files: every `<BiomeRule>`
/// with its species mix.
std::vector<VegBiomeRule> ReadVegetationBiomeRules(const std::string& path);

}  // namespace sdl3cpp::fs2024
