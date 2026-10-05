#include "services/interfaces/workflow/racer/data/racer_track_plot.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

namespace sdl3cpp::services::impl {
namespace {

using Colour = std::array<std::uint8_t, 3>;
constexpr Colour kMain{255, 255, 255};
constexpr Colour kBranch{255, 150, 40};
constexpr int kSteps = 24;

struct Canvas {
    std::vector<std::uint8_t>& rgba;
    int size;
    float minX, minY, scale;

    void Plot(const RacerVec3& p, const Colour& c) {
        const int x = static_cast<int>(8 + (p.x - minX) * scale);
        const int y = static_cast<int>(8 + (p.y - minY) * scale);
        if (x < 0 || y < 0 || x >= size || y >= size) return;
        const std::size_t at = 4 * (static_cast<std::size_t>(y) * size + x);
        rgba[at] = c[0];
        rgba[at + 1] = c[1];
        rgba[at + 2] = c[2];
        rgba[at + 3] = 255;
    }
};

void Curve(Canvas& canvas, const RacerSplineSegment& from,
           const RacerSplineSegment& to, const Colour& colour) {
    for (int s = 0; s <= kSteps * 8; ++s) {
        canvas.Plot(RacerSplinePoint(from, to, s / (kSteps * 8.f)), colour);
    }
}

}  // namespace

std::vector<std::uint8_t> PlotRacerTrack(
    const std::vector<RacerSplineSegment>& segments, int size) {
    std::vector<std::uint8_t> rgba(
        4 * static_cast<std::size_t>(size) * size, 0);
    if (segments.size() < 2 || size < 32) return rgba;
    float minX = segments[0].knot.x, maxX = minX;
    float minY = segments[0].knot.y, maxY = minY;
    for (const auto& s : segments) {
        minX = std::min(minX, s.knot.x);
        maxX = std::max(maxX, s.knot.x);
        minY = std::min(minY, s.knot.y);
        maxY = std::max(maxY, s.knot.y);
    }
    const float span = std::max({maxX - minX, maxY - minY, 1.f});
    Canvas canvas{rgba, size, minX, minY, (size - 16) / span};

    const auto main = RacerSplineMainLoop(segments);
    const int count = static_cast<int>(segments.size());
    for (int i = 0; i < count; ++i) {
        for (int k = 0; k < segments[i].successorCount && k < 2; ++k) {
            const int to = segments[i].successors[k];
            if (to < 0 || to >= count) continue;
            const bool onMain =
                k == 0 && std::find(main.begin(), main.end(), i) != main.end();
            Curve(canvas, segments[i], segments[to],
                  onMain ? kMain : kBranch);
        }
    }
    return rgba;
}

}  // namespace sdl3cpp::services::impl
