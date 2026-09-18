#pragma once

#include "services/interfaces/workflow/fs2024/assemble/fs2024_cgl_pyramid.hpp"
#include "services/interfaces/workflow/fs2024/data/fs2024_shared_cache.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// What covers the ground anywhere on Earth, from FS2024's own
/// ground-cover layer (lcg). Safe to share between loader threads.
class Fs2024ClassSampler {
public:
    explicit Fs2024ClassSampler(const std::string& cglRoot);

    /// FS2024's land class at Web Mercator (u, v): the green channel of
    /// the nearest lcg sample over ten -- GlobeLand30's classes, the
    /// same numbering its biome table uses (1 cultivated, 2 forest,
    /// 3 grassland, 4 shrubland, 5 wetland, 8 artificial, 9 bare).
    /// Finest level first (12, ~24 m at London) down to 8; 0 where it
    /// has no tile, which is open water -- that comes from the vector
    /// layer instead.
    std::uint8_t ClassAt(double u, double v);

private:
    struct Image {
        int width = 0, height = 0;
        std::vector<std::uint8_t> classes;  ///< already divided by ten
    };
    std::shared_ptr<const Image> ImageAt(int level, int x, int y);

    Fs2024CglPyramid pyramid_;
    sdl3cpp::fs2024::SharedCache<std::uint64_t, Image> images_;
};

}  // namespace sdl3cpp::services::impl
