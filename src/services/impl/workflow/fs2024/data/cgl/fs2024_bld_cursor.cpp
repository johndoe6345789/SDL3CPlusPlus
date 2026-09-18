#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_cursor.hpp"

#include <stdexcept>

namespace sdl3cpp::fs2024 {

std::uint8_t BldCursor::U8() {
    if (at_ >= size_) throw std::runtime_error("BLD tile: ran off the end");
    return data_[at_++];
}

std::uint16_t BldCursor::U16() {
    const std::uint16_t low = U8();
    return static_cast<std::uint16_t>(low | (U8() << 8));
}

std::uint32_t BldCursor::Varint8() {
    const std::uint8_t first = U8();
    if ((first & 0x80) == 0) return first;
    return static_cast<std::uint32_t>((first & 0x7F) << 8) | U8();
}

std::uint32_t BldCursor::Varint16() {
    const std::uint16_t first = U16();
    if ((first & 0x8000) == 0) return first;
    return (static_cast<std::uint32_t>(first & 0x7FFF) << 16) | U16();
}

std::int32_t BldCursor::DeltaVarint16() {
    const std::uint32_t value = Varint16();
    return (value & 1) ? -static_cast<std::int32_t>(value >> 1)
                       : static_cast<std::int32_t>(value >> 1);
}

void BldCursor::Skip(std::size_t count) {
    if (size_ - at_ < count) {
        throw std::runtime_error("BLD tile: ran off the end");
    }
    at_ += count;
}

}  // namespace sdl3cpp::fs2024
