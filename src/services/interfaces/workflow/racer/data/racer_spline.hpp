#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// A position in the game's own space: x and y span the ground, z is up.
struct RacerVec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

/**
 * One segment of a track spline (84 bytes: 68 of data, then a 16-byte
 * trailer). The curve is a cubic Bezier: from this knot, leaving along
 * `controlAfter`, to the next knot arriving along its `controlBefore`.
 * A segment has one or two successors and predecessors; two mean a
 * fork or a join (an alternate route). Alternate routes are stored
 * running back toward the fork, so the links are best read as edges.
 */
struct RacerSplineSegment {
    std::uint16_t successorCount = 0;
    std::uint16_t predecessorCount = 0;
    std::array<std::int16_t, 2> successors{{-1, -1}};
    std::array<std::int16_t, 2> predecessors{{-1, -1}};
    RacerVec3 knot;
    RacerVec3 controlBefore;
    RacerVec3 controlAfter;
};

constexpr std::size_t kRacerSplineHeaderBytes = 16;
constexpr std::size_t kRacerSplineSegmentBytes = 84;

/// Reads a spline item: a 16-byte header whose word at +4 is the
/// segment count, then the segments. Empty if the item is too short.
std::vector<RacerSplineSegment> ReadRacerSpline(
    const std::vector<std::uint8_t>& item);

/// The main lap: segment indices from 0 following each first
/// successor until the route returns to 0. Empty if it never does.
std::vector<int> RacerSplineMainLoop(
    const std::vector<RacerSplineSegment>& segments);

/// A point on the Bezier from segment `from` to segment `to`, t in 0..1.
RacerVec3 RacerSplinePoint(const RacerSplineSegment& from,
                           const RacerSplineSegment& to, float t);

}  // namespace sdl3cpp::services::impl
