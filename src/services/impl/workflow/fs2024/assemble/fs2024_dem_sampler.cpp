#include "services/interfaces/workflow/fs2024/assemble/fs2024_dem_sampler.hpp"

#include "services/interfaces/workflow/fs2024/data/dem/fs2024_dem_calibration.hpp"
#include "services/interfaces/workflow/fs2024/data/dem/fs2024_dem_cgl_tile.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kFinest = 10, kCoarsest = 6, kSamples = 257;
constexpr std::size_t kMaxGrids = 96;  ///< ~25 MB of floats

std::uint64_t GridKey(int level, int x, int y) {
    return (static_cast<std::uint64_t>(level) << 56) |
           (static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) << 28) |
           static_cast<std::uint32_t>(y);
}

float Bilinear(const std::vector<float>& grid, double fx, double fy) {
    const int c = std::min(static_cast<int>(fx), kSamples - 2);
    const int r = std::min(static_cast<int>(fy), kSamples - 2);
    const float tx = static_cast<float>(fx - c);
    const float ty = static_cast<float>(fy - r);
    const auto at = [&](int cc, int rr) {
        return grid[static_cast<std::size_t>(rr) * kSamples + cc];
    };
    const float top = at(c, r) + (at(c + 1, r) - at(c, r)) * tx;
    const float bottom = at(c, r + 1) + (at(c + 1, r + 1) - at(c, r + 1)) * tx;
    return top + (bottom - top) * ty;
}

}  // namespace

Fs2024DemSampler::Fs2024DemSampler(const std::string& cglRoot)
    : pyramid_(cglRoot, "dem"), grids_(kMaxGrids) {}

std::shared_ptr<const std::vector<float>> Fs2024DemSampler::Grid(int level,
                                                                 int x,
                                                                 int y) {
    return grids_.Get(GridKey(level, x, y), [&] {
        std::shared_ptr<std::vector<float>> grid;
        const auto blob = pyramid_.Read(level, x, y);
        if (blob.empty()) return grid;
        const auto dem = sdl3cpp::fs2024::DecodeFs2024DemTile(blob);
        if (dem.width == kSamples && dem.height == kSamples) {
            grid = std::make_shared<std::vector<float>>(dem.raw.size());
            std::transform(dem.raw.begin(), dem.raw.end(), grid->begin(),
                           sdl3cpp::fs2024::Fs2024DemMetres);
        }
        return grid;
    });
}

float Fs2024DemSampler::MetresAt(double u, double v) {
    for (int level = kFinest; level >= kCoarsest; --level) {
        const double side = static_cast<double>(1 << level);
        const int x = static_cast<int>(std::floor(u * side));
        const int y = static_cast<int>(std::floor(v * side));
        const auto grid = Grid(level, x, y);
        if (!grid) continue;
        return Bilinear(*grid, (u * side - x) * (kSamples - 1),
                        (v * side - y) * (kSamples - 1));
    }
    return 0.f;
}

}  // namespace sdl3cpp::services::impl
