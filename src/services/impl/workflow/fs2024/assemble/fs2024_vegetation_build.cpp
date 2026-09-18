#include "services/interfaces/workflow/fs2024/assemble/fs2024_vegetation_build.hpp"

#include "services/interfaces/workflow/fs2024/assemble/fs2024_vegetation_pick.hpp"
#include "services/interfaces/workflow/fs2024/assemble/fs2024_vegetation_place_helpers.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

namespace f = sdl3cpp::fs2024;

constexpr int kForest = 2;
constexpr int kShrub = 4;

void PlaceOn(int landClass, const f::VegetationLibrary& library,
            const std::vector<std::uint8_t>& classes, int classSize,
            const Fs2024Heightfield& field, float tileSize,
            double centreLatitude, std::uint64_t seed,
            std::vector<Fs2024VegetationGroupCpu>& groups) {
    const std::string biomeName =
        Fs2024VegetationBiomeFor(landClass, centreLatitude);
    if (biomeName.empty()) return;
    const f::VegBiomeRule* rule = library.BiomeRule(biomeName);
    if (!rule) return;
    const float spacing = Fs2024VegSpacingFor(rule->instancesPerHectare);
    const int cells = std::max(static_cast<int>(tileSize / spacing), 1);
    for (int r = 0; r < cells; ++r) {
        for (int c = 0; c < cells; ++c) {
            float x = 0.f, z = 0.f;
            if (!Fs2024VegSpot(landClass, classes, classSize, tileSize,
                               spacing, seed, r, c, x, z)) {
                continue;
            }
            Fs2024VegPlaceOne(library, *rule, landClass, x, z, field, seed,
                              r, c, groups);
        }
    }
}

}  // namespace

std::vector<Fs2024VegetationGroupCpu> BuildFs2024Vegetation(
    const f::VegetationLibrary& library,
    const std::vector<std::uint8_t>& classes, int classSize,
    const Fs2024Heightfield& field, float tileSize, double centreLatitude,
    std::uint64_t tileSeed) {
    std::vector<Fs2024VegetationGroupCpu> groups;
    for (const int landClass : {kForest, kShrub}) {
        PlaceOn(landClass, library, classes, classSize, field, tileSize,
               centreLatitude, tileSeed, groups);
    }
    return groups;
}

}  // namespace sdl3cpp::services::impl
