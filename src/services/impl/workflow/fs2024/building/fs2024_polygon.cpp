#include "services/interfaces/workflow/fs2024/building/fs2024_polygon.hpp"

namespace sdl3cpp::services::impl {
namespace {

float Cross(const Point2& a, const Point2& b, const Point2& c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool PointInTriangle(const Point2& p, const Point2& a, const Point2& b,
                     const Point2& c) {
    const float d1 = Cross(a, b, p), d2 = Cross(b, c, p), d3 = Cross(c, a, p);
    const bool hasNeg = d1 < 0 || d2 < 0 || d3 < 0;
    const bool hasPos = d1 > 0 || d2 > 0 || d3 > 0;
    return !(hasNeg && hasPos);
}

/// Whether clipping ear (prev, i, next) is valid: the ear must turn
/// the same way as the polygon overall, and no other remaining vertex
/// may lie inside the triangle it would cut off.
bool IsEar(const std::vector<Point2>& polygon,
          const std::vector<std::uint32_t>& active, std::size_t i,
          bool clockwise) {
    const std::size_t n = active.size();
    const Point2& prev = polygon[active[(i + n - 1) % n]];
    const Point2& curr = polygon[active[i]];
    const Point2& next = polygon[active[(i + 1) % n]];
    const float turn = Cross(prev, curr, next);
    if (clockwise ? turn > 0.f : turn < 0.f) return false;

    for (std::size_t k = 0; k < n; ++k) {
        if (k == i || k == (i + n - 1) % n || k == (i + 1) % n) continue;
        if (PointInTriangle(polygon[active[k]], prev, curr, next)) {
            return false;
        }
    }
    return true;
}

}  // namespace

std::vector<std::uint32_t> TriangulatePolygon(
    const std::vector<Point2>& polygon) {
    std::vector<std::uint32_t> triangles;
    if (polygon.size() < 3) return triangles;

    // Points repeated at the start/end (a closed OSM ring) confuse the
    // ear test with a zero-area sliver; drop the duplicate.
    std::vector<std::uint32_t> active;
    active.reserve(polygon.size());
    for (std::uint32_t i = 0; i < polygon.size(); ++i) active.push_back(i);
    if (active.size() > 1) {
        const Point2& first = polygon[active.front()];
        const Point2& last = polygon[active.back()];
        if (first.x == last.x && first.y == last.y) active.pop_back();
    }
    if (active.size() < 3) return triangles;

    float signedArea = 0.f;
    for (std::size_t i = 0; i < active.size(); ++i) {
        const Point2& a = polygon[active[i]];
        const Point2& b = polygon[active[(i + 1) % active.size()]];
        signedArea += a.x * b.y - b.x * a.y;
    }
    const bool clockwise = signedArea < 0.f;

    // Each pass removes exactly one vertex at a found ear; a pass that
    // finds none (a degenerate or self-intersecting input) stops
    // rather than spinning forever on a polygon that cannot finish.
    while (active.size() > 3) {
        bool clipped = false;
        for (std::size_t i = 0; i < active.size(); ++i) {
            if (!IsEar(polygon, active, i, clockwise)) continue;
            const std::size_t n = active.size();
            triangles.push_back(active[(i + n - 1) % n]);
            triangles.push_back(active[i]);
            triangles.push_back(active[(i + 1) % n]);
            active.erase(active.begin() + static_cast<long>(i));
            clipped = true;
            break;
        }
        if (!clipped) break;
    }
    if (active.size() == 3) {
        triangles.push_back(active[0]);
        triangles.push_back(active[1]);
        triangles.push_back(active[2]);
    }
    return triangles;
}

}  // namespace sdl3cpp::services::impl
