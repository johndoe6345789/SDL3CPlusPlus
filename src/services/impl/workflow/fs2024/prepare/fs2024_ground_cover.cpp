#include "services/interfaces/workflow/fs2024/prepare/fs2024_ground_cover.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::fs2024 {
namespace {

using Colour = std::array<float, 3>;
constexpr Colour kMeadow{92.f, 110.f, 56.f};
constexpr Colour kForest{44.f, 62.f, 36.f};
constexpr Colour kAlpine{112.f, 112.f, 74.f};
constexpr Colour kRock{118.f, 114.f, 106.f};
constexpr Colour kSnow{232.f, 236.f, 240.f};

float Ramp(float value, float low, float high) {
    const float t = std::clamp((value - low) / (high - low), 0.f, 1.f);
    return t * t * (3.f - 2.f * t);  // smoothstep
}

Colour Mix(const Colour& a, const Colour& b, float t) {
    return {a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t,
           a[2] + (b[2] - a[2]) * t};
}

/// Separable box blur, run twice to roughly approximate a Gaussian --
/// good enough to break up DSM noise, not meant to be exact.
std::vector<float> BoxBlur(const std::vector<float>& values, int cells,
                          int radius) {
    std::vector<float> src = values, dst(values.size());
    for (int pass = 0; pass < 2; ++pass) {
        for (int row = 0; row < cells; ++row) {
            for (int col = 0; col < cells; ++col) {
                float sum = 0.f;
                int count = 0;
                for (int d = -radius; d <= radius; ++d) {
                    const int c = std::clamp(col + d, 0, cells - 1);
                    sum += src[static_cast<std::size_t>(row) * cells + c];
                    ++count;
                }
                dst[static_cast<std::size_t>(row) * cells + col] =
                    sum / static_cast<float>(count);
            }
        }
        std::swap(src, dst);
        for (int col = 0; col < cells; ++col) {
            for (int row = 0; row < cells; ++row) {
                float sum = 0.f;
                int count = 0;
                for (int d = -radius; d <= radius; ++d) {
                    const int r = std::clamp(row + d, 0, cells - 1);
                    sum += src[static_cast<std::size_t>(r) * cells + col];
                    ++count;
                }
                dst[static_cast<std::size_t>(row) * cells + col] =
                    sum / static_cast<float>(count);
            }
        }
        std::swap(src, dst);
    }
    return src;
}

}  // namespace

std::array<std::uint8_t, 3> CoverColour(float altitude, float slopeDegrees) {
    Colour colour = Mix(kMeadow, kForest, Ramp(slopeDegrees, 6.f, 14.f));
    colour = Mix(colour, kAlpine, Ramp(altitude, 1700.f, 2000.f));
    colour = Mix(colour, kRock, Ramp(slopeDegrees, 34.f, 44.f));
    colour = Mix(colour, kRock, Ramp(altitude, 2250.f, 2450.f));
    const float snow =
        Ramp(altitude, 2500.f, 2700.f) * (1.f - Ramp(slopeDegrees, 38.f, 50.f));
    colour = Mix(colour, kSnow, snow);
    return {static_cast<std::uint8_t>(std::clamp(colour[0], 0.f, 255.f)),
           static_cast<std::uint8_t>(std::clamp(colour[1], 0.f, 255.f)),
           static_cast<std::uint8_t>(std::clamp(colour[2], 0.f, 255.f))};
}

std::vector<float> ComputeSlopeDegrees(const std::vector<float>& heights,
                                       int cells, float spacing) {
    const std::vector<float> smoothed = BoxBlur(heights, cells, 2);
    std::vector<float> slope(heights.size());
    for (int row = 0; row < cells; ++row) {
        const int r0 = std::max(row - 1, 0), r1 = std::min(row + 1,
                                                           cells - 1);
        for (int col = 0; col < cells; ++col) {
            const int c0 = std::max(col - 1, 0), c1 = std::min(col + 1,
                                                               cells - 1);
            const float dz =
                (smoothed[static_cast<std::size_t>(row) * cells + c1] -
                smoothed[static_cast<std::size_t>(row) * cells + c0]) /
                (static_cast<float>(c1 - c0) * spacing);
            const float dx =
                (smoothed[static_cast<std::size_t>(r1) * cells + col] -
                smoothed[static_cast<std::size_t>(r0) * cells + col]) /
                (static_cast<float>(r1 - r0) * spacing);
            slope[static_cast<std::size_t>(row) * cells + col] =
                std::atan(std::hypot(dx, dz)) * 180.f / 3.14159265f;
        }
    }
    return slope;
}

void FlattenTowards(std::vector<float>& heights, int cells,
                   const std::vector<std::uint8_t>& mask, float target) {
    std::vector<float> soft(mask.size());
    for (std::size_t i = 0; i < mask.size(); ++i) {
        soft[i] = mask[i];
    }
    // A cheap dilate (grow the mask outward a couple of cells) before
    // blurring, so the flattened skirt reaches a little past the
    // pavement itself rather than stopping exactly at its edge.
    std::vector<float> grown = soft;
    for (int row = 0; row < cells; ++row) {
        for (int col = 0; col < cells; ++col) {
            float best = 0.f;
            for (int dr = -2; dr <= 2; ++dr) {
                for (int dc = -2; dc <= 2; ++dc) {
                    const int r = std::clamp(row + dr, 0, cells - 1);
                    const int c = std::clamp(col + dc, 0, cells - 1);
                    best = std::max(
                        best, soft[static_cast<std::size_t>(r) * cells + c]);
                }
            }
            grown[static_cast<std::size_t>(row) * cells + col] = best;
        }
    }
    const std::vector<float> blurred = BoxBlur(grown, cells, 3);
    for (std::size_t i = 0; i < heights.size(); ++i) {
        const float weight = std::clamp(blurred[i] / 255.f * 1.6f, 0.f, 1.f);
        heights[i] += (target - heights[i]) * weight;
    }
}

}  // namespace sdl3cpp::fs2024
