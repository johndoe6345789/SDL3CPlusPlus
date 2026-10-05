#include "services/interfaces/workflow/racer/data/racer_spline.hpp"

#include <cstring>

namespace sdl3cpp::services::impl {
namespace {

std::uint32_t ReadBe32(const std::uint8_t* at) {
    return (static_cast<std::uint32_t>(at[0]) << 24) |
           (static_cast<std::uint32_t>(at[1]) << 16) |
           (static_cast<std::uint32_t>(at[2]) << 8) |
           static_cast<std::uint32_t>(at[3]);
}

float ReadBeFloat(const std::uint8_t* at) {
    const std::uint32_t bits = ReadBe32(at);
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

RacerVec3 ReadPoint(const std::uint8_t* at) {
    // Stored as x, z, then height.
    return {ReadBeFloat(at), ReadBeFloat(at + 8), ReadBeFloat(at + 4)};
}

}  // namespace

std::vector<RacerSplineRecord> ReadRacerSpline(const std::uint8_t* data,
                                               std::size_t size) {
    if (size < 8) return {};
    const std::size_t count = ReadBe32(data + 4);
    if (size < count * kRacerSplineRecordBytes) return {};

    std::vector<RacerSplineRecord> records;
    records.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const std::uint8_t* base = data + i * kRacerSplineRecordBytes;
        RacerSplineRecord record;
        record.id = ReadBe32(base + 20) >> 16;
        record.prevId = ReadBe32(base + 24) >> 16;
        // Words 8-10, 14-16 and 17-19 hold three points (word 13 is 1.0).
        record.points[0] = ReadPoint(base + 32);
        record.points[1] = ReadPoint(base + 56);
        record.points[2] = ReadPoint(base + 68);
        records.push_back(record);
    }
    return records;
}

}  // namespace sdl3cpp::services::impl
