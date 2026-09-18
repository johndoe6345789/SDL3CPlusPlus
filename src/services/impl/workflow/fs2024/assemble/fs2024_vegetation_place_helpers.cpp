#include "services/interfaces/workflow/fs2024/assemble/fs2024_vegetation_place_helpers.hpp"

#include "services/interfaces/workflow/fs2024/assemble/fs2024_vegetation_hash.hpp"
#include "services/interfaces/workflow/fs2024/assemble/fs2024_vegetation_quad.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

/// FS2024's own densest fallback rule (700/ha, ~3.8 m real spacing)
/// would put ~160,000 trees on one finest tile. This engine budgets a
/// far sparser, but still relatively-scaled, spacing instead: a real
/// stand of vegetation, not a faithful density.
constexpr float kRefDensityPerHectare = 700.f;
constexpr float kRefSpacingMetres = 22.f;

}  // namespace

float Fs2024VegSpacingFor(float instancesPerHectare) {
    return kRefSpacingMetres *
          std::sqrt(kRefDensityPerHectare / std::max(instancesPerHectare, 1.f));
}

int Fs2024VegClassAt(const std::vector<std::uint8_t>& classes, int classSize,
                     float x, float z, float tileSize) {
    const int tx = std::clamp(static_cast<int>(x / tileSize * classSize), 0,
                              classSize - 1);
    const int tz = std::clamp(static_cast<int>(z / tileSize * classSize), 0,
                              classSize - 1);
    return classes[static_cast<std::size_t>(tz) * classSize + tx];
}

bool Fs2024VegSpot(int landClass, const std::vector<std::uint8_t>& classes,
                   int classSize, float tileSize, float spacing,
                   std::uint64_t seed, int r, int c, float& x, float& z) {
    const float jx = Fs2024VegHash(seed, c, r, landClass);
    const float jz = Fs2024VegHash(seed, c, r, landClass + 1000);
    x = (c + 0.15f + 0.7f * jx) * spacing;
    z = (r + 0.15f + 0.7f * jz) * spacing;
    if (x >= tileSize || z >= tileSize) return false;
    return Fs2024VegClassAt(classes, classSize, x, z, tileSize) == landClass;
}

Fs2024VegetationGroupCpu& Fs2024VegGroupFor(
    std::vector<Fs2024VegetationGroupCpu>& groups,
    const sdl3cpp::fs2024::VegSpecies& species, const std::string& albedo) {
    for (auto& group : groups) {
        if (group.speciesName == species.name) return group;
    }
    groups.push_back({species.name, albedo, {}});
    Fs2024VegetationGroupCpu& group = groups.back();
    group.mesh.min = glm::vec3(1e30f);
    group.mesh.max = glm::vec3(-1e30f);
    return group;
}

}  // namespace sdl3cpp::services::impl
