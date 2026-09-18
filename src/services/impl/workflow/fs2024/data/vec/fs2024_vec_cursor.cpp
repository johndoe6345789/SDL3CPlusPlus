#include "services/interfaces/workflow/fs2024/data/vec/fs2024_vec_cursor.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace sdl3cpp::fs2024 {

void VecCursor::Need(std::size_t bytes) const {
    if (at_ + bytes > blob_.size()) throw std::runtime_error("vec: overrun");
}

std::uint8_t VecCursor::U8() {
    Need(1);
    return blob_[at_++];
}

std::uint16_t VecCursor::U16() {
    Need(2);
    std::uint16_t value = 0;
    std::memcpy(&value, &blob_[at_], 2);
    at_ += 2;
    return value;
}

std::uint32_t VecCursor::U32() {
    Need(4);
    std::uint32_t value = 0;
    std::memcpy(&value, &blob_[at_], 4);
    at_ += 4;
    return value;
}

void VecCursor::Skip(std::size_t bytes) {
    Need(bytes);
    at_ += bytes;
}

std::vector<int> VecCursor::ClassMask() {
    const std::uint32_t mask = U32();
    std::vector<int> classes;
    std::uint16_t previous = 0;
    for (int bit = 0; bit < 32; ++bit) {
        if (!(mask >> bit & 1u)) continue;
        const std::uint16_t cumulative = U16();
        if (cumulative < previous) {
            throw std::runtime_error("vec: class counts not monotonic");
        }
        classes.insert(classes.end(), cumulative - previous, bit);
        previous = cumulative;
    }
    if (mask != 0 && U16() != 0) {
        throw std::runtime_error("vec: header terminator");
    }
    return classes;
}

std::vector<VecPoint> VecCursor::Points(std::size_t count) {
    Need(count * 4);
    std::vector<VecPoint> points(count);
    for (VecPoint& point : points) {
        std::memcpy(&point.x, &blob_[at_], 2);
        std::memcpy(&point.y, &blob_[at_ + 2], 2);
        at_ += 4;
    }
    return points;
}

bool VecCursor::AtPaddedEnd() const {
    return Left() <= 6 &&
           std::all_of(blob_.begin() + static_cast<long>(at_), blob_.end(),
                       [](std::uint8_t b) { return b == 0; });
}

}  // namespace sdl3cpp::fs2024
