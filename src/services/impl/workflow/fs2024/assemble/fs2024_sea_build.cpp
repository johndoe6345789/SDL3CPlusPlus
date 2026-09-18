#include "services/interfaces/workflow/fs2024/assemble/fs2024_sea_build.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

/// FS2024's DEM holds open water at one value -- the sea off Brighton
/// is -2.33 m sample after sample, Lake Geneva within 2 cm -- where even
/// the flattest land varies.
constexpr float kStill = 0.02f;
constexpr float kBedDepth = 3.f;  ///< ground under the water, below it

bool OpenWater(const Fs2024Heightfield& field, int c, int r, float& level) {
    const float corners[4] = {field.At(c, r), field.At(c + 1, r),
                              field.At(c, r + 1), field.At(c + 1, r + 1)};
    const auto [lo, hi] = std::minmax_element(corners, corners + 4);
    level = *lo;
    return *hi - *lo <= kStill;
}

}  // namespace

void AddFs2024OpenWater(const Fs2024TileShapes& shapes,
                        Fs2024Heightfield& field,
                        Fs2024TerrainChunkMesh& water) {
    std::vector<float> carved = field.heights;
    for (int r = 0; r + 1 < field.rows; ++r) {
        for (int c = 0; c + 1 < field.columns; ++c) {
            float level = 0.f;
            const glm::vec3 middle = field.Position(c, r) +
                                     0.5f * glm::vec3(field.spacing, 0.f,
                                                      field.spacing);
            if (shapes.Maps(middle.x, middle.z) ||
                !OpenWater(field, c, r, level)) {
                continue;
            }
            const auto first =
                static_cast<std::uint32_t>(water.vertices.size());
            for (const auto [dc, dr] : {std::pair{0, 0}, std::pair{1, 0},
                                        std::pair{0, 1}, std::pair{1, 1}}) {
                const glm::vec3 p = field.Position(c + dc, r + dr);
                const glm::vec3 at(p.x, level, p.z);
                water.vertices.push_back(BspRenderVertex{
                    at.x, at.y, at.z, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f});
                water.min = glm::min(water.min, at);
                water.max = glm::max(water.max, at);
                float& h = carved[static_cast<std::size_t>(r + dr) *
                                      field.columns + c + dc];
                h = std::min(h, level - kBedDepth);
            }
            water.indices.insert(water.indices.end(),
                                 {first, first + 2, first + 1, first + 1,
                                  first + 2, first + 3});
        }
    }
    field.heights = std::move(carved);
    field.minHeight = *std::min_element(field.heights.begin(),
                                        field.heights.end());
}

}  // namespace sdl3cpp::services::impl
