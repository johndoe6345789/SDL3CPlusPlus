#include "services/interfaces/workflow/racer/data/racer_track_plot.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

struct Bounds {
    float minX = 0.0f, maxX = 0.0f, minZ = 0.0f, maxZ = 0.0f;
};

Bounds FindBounds(const std::vector<RacerSplineRecord>& records) {
    Bounds bounds;
    bool first = true;
    for (const auto& record : records) {
        for (const auto& p : record.points) {
            if (first) {
                bounds = {p.x, p.x, p.z, p.z};
                first = false;
                continue;
            }
            bounds.minX = std::min(bounds.minX, p.x);
            bounds.maxX = std::max(bounds.maxX, p.x);
            bounds.minZ = std::min(bounds.minZ, p.z);
            bounds.maxZ = std::max(bounds.maxZ, p.z);
        }
    }
    return bounds;
}

void Put(std::vector<std::uint8_t>& rgba, int size, int x, int y,
         const std::uint8_t* colour) {
    if (x < 0 || y < 0 || x >= size || y >= size) return;
    const std::size_t at = 4 * (static_cast<std::size_t>(y) * size + x);
    rgba[at] = colour[0];
    rgba[at + 1] = colour[1];
    rgba[at + 2] = colour[2];
    rgba[at + 3] = 255;
}

// Plain DDA line; enough for a diagnostic picture.
void Line(std::vector<std::uint8_t>& rgba, int size, float x0, float y0,
          float x1, float y1, const std::uint8_t* colour) {
    const int steps = static_cast<int>(
        std::max(std::fabs(x1 - x0), std::fabs(y1 - y0)) + 1.0f);
    for (int s = 0; s <= steps; ++s) {
        const float t = static_cast<float>(s) / steps;
        Put(rgba, size, static_cast<int>(x0 + (x1 - x0) * t),
            static_cast<int>(y0 + (y1 - y0) * t), colour);
    }
}

}  // namespace

std::vector<std::uint8_t> PlotRacerTrack(
    const std::vector<RacerSplineRecord>& records, int size) {
    std::vector<std::uint8_t> rgba(
        4 * static_cast<std::size_t>(size) * size, 0);
    if (records.size() < 2 || size < 2) return rgba;

    const Bounds b = FindBounds(records);
    const float span = std::max({b.maxX - b.minX, b.maxZ - b.minZ, 1.0f});
    const float scale = (size - 16) / span;
    auto toPixel = [&](const RacerVec3& p) {
        return std::pair<float, float>{8 + (p.x - b.minX) * scale,
                                       8 + (p.z - b.minZ) * scale};
    };
    const std::uint8_t colours[3][3] = {
        {255, 90, 90}, {90, 255, 90}, {90, 160, 255}};

    for (std::size_t lane = 0; lane < 3; ++lane) {
        for (std::size_t i = 0; i < records.size(); ++i) {
            const std::size_t next = (i + 1) % records.size();
            const auto from = toPixel(records[i].points[lane]);
            const auto to = toPixel(records[next].points[lane]);
            Line(rgba, size, from.first, from.second, to.first, to.second,
                 colours[lane]);
        }
    }
    return rgba;
}

}  // namespace sdl3cpp::services::impl
