#include "services/interfaces/workflow/fs2024/assemble/fs2024_vegetation_place_helpers.hpp"

#include "services/interfaces/workflow/fs2024/assemble/fs2024_vegetation_hash.hpp"
#include "services/interfaces/workflow/fs2024/assemble/fs2024_vegetation_quad.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {
constexpr float kCanopyAspect = 0.6f;  ///< width as a fraction of height
}  // namespace

void Fs2024VegPlaceOne(const sdl3cpp::fs2024::VegetationLibrary& library,
                       const sdl3cpp::fs2024::VegBiomeRule& rule,
                       int landClass, float x, float z,
                       const Fs2024Heightfield& field, std::uint64_t seed,
                       int r, int c,
                       std::vector<Fs2024VegetationGroupCpu>& groups) {
    namespace f = sdl3cpp::fs2024;
    const auto* ref = Fs2024VegPickWeighted(
        rule.species, [](const f::VegBiomeSpecies& s) { return s.spawnRatio; },
        Fs2024VegHash(seed, c, r, landClass + 2000));
    const f::VegSpecies* species = ref ? library.Species(ref->name) : nullptr;
    if (!species || species->variations.empty()) return;
    const std::string albedo = library.AlbedoPath(*species);
    if (albedo.empty()) return;
    const auto* variation = Fs2024VegPickWeighted(
        species->variations,
        [](const f::VegVariation& v) { return v.spawnRatio; },
        Fs2024VegHash(seed, c, r, landClass + 3000));
    if (!variation) return;
    const float sizeRoll = Fs2024VegHash(seed, c, r, landClass + 4000);
    const float height = variation->sizeMin +
                         (variation->sizeMax - variation->sizeMin) * sizeRoll;
    const int frames = std::max(variation->frames, 1);
    const int frameCol = static_cast<int>(
        Fs2024VegHash(seed, c, r, landClass + 5000) * frames) % frames;
    AppendFs2024VegetationQuad(
        Fs2024VegGroupFor(groups, *species, albedo).mesh, x, z,
        Fs2024HeightAt(field, x, z), height * kCanopyAspect, height,
        variation->relativeOffsetY * height, variation->textureIndex, frames,
        frameCol);
}

}  // namespace sdl3cpp::services::impl
