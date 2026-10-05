#include "services/interfaces/workflow/racer/data/racer_spline.hpp"

#include "services/interfaces/workflow/racer/data/racer_big_endian.hpp"

namespace sdl3cpp::services::impl {
namespace {

RacerVec3 ReadVec(const RacerBigEndianReader& r, std::size_t at) {
    return {r.F32(at), r.F32(at + 4), r.F32(at + 8)};
}

std::int16_t Link(const RacerBigEndianReader& r, std::size_t at, int slot,
                  int count) {
    return slot < count ? r.I16(at) : static_cast<std::int16_t>(-1);
}

}  // namespace

std::vector<RacerSplineSegment> ReadRacerSpline(
    const std::vector<std::uint8_t>& item) {
    const RacerBigEndianReader r(item);
    if (!r.Has(0, kRacerSplineHeaderBytes)) return {};
    const std::int32_t count = r.I32(4);
    const std::size_t bytes = kRacerSplineSegmentBytes * count;
    if (count <= 0 || !r.Has(kRacerSplineHeaderBytes, bytes)) return {};

    std::vector<RacerSplineSegment> segments(count);
    for (std::int32_t i = 0; i < count; ++i) {
        const std::size_t at =
            kRacerSplineHeaderBytes + kRacerSplineSegmentBytes * i;
        RacerSplineSegment& s = segments[i];
        // Counts at +0/+2; successor ids at +4 and +10, predecessor ids
        // at +8 and +6. Slots beyond a count hold leftover bytes.
        s.predecessorCount = r.U16(at);
        s.successorCount = r.U16(at + 2);
        s.successors = {Link(r, at + 4, 0, s.successorCount),
                        Link(r, at + 10, 1, s.successorCount)};
        s.predecessors = {Link(r, at + 8, 0, s.predecessorCount),
                          Link(r, at + 6, 1, s.predecessorCount)};
        s.knot = ReadVec(r, at + 0x10);
        s.controlBefore = ReadVec(r, at + 0x28);
        s.controlAfter = ReadVec(r, at + 0x34);
    }
    return segments;
}

std::vector<int> RacerSplineMainLoop(
    const std::vector<RacerSplineSegment>& segments) {
    const int count = static_cast<int>(segments.size());
    std::vector<int> loop;
    std::vector<bool> seen(segments.size(), false);
    for (int at = 0; at >= 0 && at < count && !seen[at];) {
        seen[at] = true;
        loop.push_back(at);
        at = segments[at].successors[0];
        if (at == 0) return loop;
    }
    return {};
}

RacerVec3 RacerSplinePoint(const RacerSplineSegment& from,
                           const RacerSplineSegment& to, float t) {
    const float u = 1.f - t;
    const float a = u * u * u, b = 3 * u * u * t, c = 3 * u * t * t,
                d = t * t * t;
    auto mix = [&](float p0, float p1, float p2, float p3) {
        return a * p0 + b * p1 + c * p2 + d * p3;
    };
    return {mix(from.knot.x, from.controlAfter.x, to.controlBefore.x,
                to.knot.x),
            mix(from.knot.y, from.controlAfter.y, to.controlBefore.y,
                to.knot.y),
            mix(from.knot.z, from.controlAfter.z, to.controlBefore.z,
                to.knot.z)};
}

}  // namespace sdl3cpp::services::impl
