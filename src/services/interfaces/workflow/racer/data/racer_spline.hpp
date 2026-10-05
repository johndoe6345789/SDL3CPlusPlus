#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

struct RacerVec3 {
    float x = 0.0f;
    float y = 0.0f;  // height
    float z = 0.0f;
};

/// One 84-byte record of a spline entry. `id` and `prevId` form a ring:
/// each record names its own 1-based slot and the slot before it, and the
/// last record wraps to slot 0. `points` are three positions on the
/// track; their roles are not yet confirmed (see the racer README).
struct RacerSplineRecord {
    std::uint32_t id = 0;
    std::uint32_t prevId = 0;
    std::array<RacerVec3, 3> points{};
};

/// Bytes per record in a spline entry.
constexpr std::size_t kRacerSplineRecordBytes = 84;

/// Reads a spline entry: a header whose second word is the record
/// count, then the records. Returns an empty list if the entry is too
/// short for the count it declares.
std::vector<RacerSplineRecord> ReadRacerSpline(const std::uint8_t* data,
                                               std::size_t size);

}  // namespace sdl3cpp::services::impl
