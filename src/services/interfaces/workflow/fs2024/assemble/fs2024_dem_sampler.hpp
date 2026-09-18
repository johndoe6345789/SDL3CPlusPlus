#pragma once

#include "services/interfaces/workflow/fs2024/assemble/fs2024_cgl_pyramid.hpp"
#include "services/interfaces/workflow/fs2024/data/fs2024_shared_cache.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// Ground height anywhere on Earth from FS2024's own DEM. Safe to share
/// between loader threads.
class Fs2024DemSampler {
public:
    explicit Fs2024DemSampler(const std::string& cglRoot);

    /// Metres above sea level at Web Mercator (u, v): bilinear between
    /// calibrated samples of the finest level FS2024 holds there (10,
    /// ~95 m at London, falling back to 6). Zero where it has no tile
    /// at any level -- open ocean.
    float MetresAt(double u, double v);

private:
    /// A decoded, calibrated 257 x 257 tile; null when absent.
    std::shared_ptr<const std::vector<float>> Grid(int level, int x, int y);

    Fs2024CglPyramid pyramid_;
    sdl3cpp::fs2024::SharedCache<std::uint64_t, std::vector<float>> grids_;
};

}  // namespace sdl3cpp::services::impl
