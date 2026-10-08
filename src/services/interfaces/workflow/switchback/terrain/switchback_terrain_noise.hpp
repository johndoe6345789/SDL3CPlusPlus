#pragma once

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Smooth seeded value noise over the unit square, summed over three octaves.
/// The same seed gives the same surface on every platform.
class SwitchbackTerrainNoise {
public:
    explicit SwitchbackTerrainNoise(std::uint64_t seed);

    /// Returns a value in roughly -1..1 for u and v in 0..1.
    float Sample(float u, float v) const;

private:
    struct Octave {
        int cells = 0;
        std::vector<float> lattice;
    };
    std::vector<Octave> octaves_;
};

}  // namespace sdl3cpp::services::impl
