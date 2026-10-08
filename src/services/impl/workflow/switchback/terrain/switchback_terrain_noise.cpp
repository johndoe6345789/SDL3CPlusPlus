#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_noise.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kOctaves = 3;
constexpr int kBaseCells = 4;

std::uint64_t NextRandom(std::uint64_t& state) {
    state += 0x9E3779B97F4A7C15ull;
    std::uint64_t z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

float Smooth(float t) { return t * t * (3.f - 2.f * t); }

}  // namespace

SwitchbackTerrainNoise::SwitchbackTerrainNoise(std::uint64_t seed) {
    std::uint64_t state = seed;
    for (int octave = 0; octave < kOctaves; ++octave) {
        Octave layer;
        layer.cells = kBaseCells << octave;
        const int side = layer.cells + 1;
        layer.lattice.reserve(static_cast<std::size_t>(side) * side);
        for (int k = 0; k < side * side; ++k) {
            const double unit = static_cast<double>(NextRandom(state) >> 11) /
                                9007199254740992.0;
            layer.lattice.push_back(static_cast<float>(unit * 2.0 - 1.0));
        }
        octaves_.push_back(std::move(layer));
    }
}

float SwitchbackTerrainNoise::Sample(float u, float v) const {
    float total = 0.f;
    float weight = 0.f;
    float amplitude = 1.f;
    for (const Octave& layer : octaves_) {
        const int cells = layer.cells;
        const int side = cells + 1;
        const float x = u * static_cast<float>(cells);
        const float y = v * static_cast<float>(cells);
        const int x0 = std::min(static_cast<int>(x), cells - 1);
        const int y0 = std::min(static_cast<int>(y), cells - 1);
        const float fx = Smooth(x - static_cast<float>(x0));
        const float fy = Smooth(y - static_cast<float>(y0));
        const auto at = [&](int ix, int iy) {
            return layer.lattice[static_cast<std::size_t>(iy * side + ix)];
        };
        const float top = at(x0, y0) + (at(x0 + 1, y0) - at(x0, y0)) * fx;
        const float low =
            at(x0, y0 + 1) + (at(x0 + 1, y0 + 1) - at(x0, y0 + 1)) * fx;
        total += amplitude * (top + (low - top) * fy);
        weight += amplitude;
        amplitude *= 0.5f;
    }
    return total / weight;
}

}  // namespace sdl3cpp::services::impl
