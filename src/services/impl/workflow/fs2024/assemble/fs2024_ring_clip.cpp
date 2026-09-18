#include "services/interfaces/workflow/fs2024/assemble/fs2024_vec_shapes.hpp"
#include "services/interfaces/workflow/fs2024/assemble/fs2024_water_build.hpp"

namespace sdl3cpp::services::impl {
namespace {

/// One Sutherland-Hodgman pass: keep what is inside the half-plane
/// `inside`, cutting each crossing edge where `cross` says.
template <typename Inside, typename Cross>
std::vector<Point2> Pass(const std::vector<Point2>& ring, Inside inside,
                         Cross cross) {
    std::vector<Point2> out;
    for (std::size_t i = 0; i < ring.size(); ++i) {
        const Point2& a = ring[i];
        const Point2& b = ring[(i + 1) % ring.size()];
        const bool inA = inside(a), inB = inside(b);
        if (inA) out.push_back(a);
        if (inA != inB) out.push_back(cross(a, b));
    }
    return out;
}

Point2 AtX(const Point2& a, const Point2& b, float x) {
    const float t = (x - a.x) / (b.x - a.x);
    return {x, a.y + (b.y - a.y) * t};
}

Point2 AtY(const Point2& a, const Point2& b, float y) {
    const float t = (y - a.y) / (b.y - a.y);
    return {a.x + (b.x - a.x) * t, y};
}

}  // namespace

std::vector<Point2> ClipRingToSquare(const std::vector<Point2>& ring,
                                     float size) {
    std::vector<Point2> out = ring;
    out = Pass(out, [](const Point2& p) { return p.x >= 0.f; },
               [](const Point2& a, const Point2& b) { return AtX(a, b, 0.f); });
    out = Pass(out, [&](const Point2& p) { return p.x <= size; },
               [&](const Point2& a, const Point2& b) {
                   return AtX(a, b, size);
               });
    out = Pass(out, [](const Point2& p) { return p.y >= 0.f; },
               [](const Point2& a, const Point2& b) { return AtY(a, b, 0.f); });
    out = Pass(out, [&](const Point2& p) { return p.y <= size; },
               [&](const Point2& a, const Point2& b) {
                   return AtY(a, b, size);
               });
    return out;
}

bool Fs2024RingContains(const std::vector<Point2>& ring, const Point2& p) {
    bool in = false;
    for (std::size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++) {
        const Point2& a = ring[i];
        const Point2& b = ring[j];
        if ((a.y > p.y) != (b.y > p.y) &&
            p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x) {
            in = !in;
        }
    }
    return in;
}

}  // namespace sdl3cpp::services::impl
