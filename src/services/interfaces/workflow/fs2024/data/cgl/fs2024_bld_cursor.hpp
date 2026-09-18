#pragma once

#include <cstddef>
#include <cstdint>

namespace sdl3cpp::fs2024 {

/// Reads the primitives FS2024's building tiles are built from. Two
/// varint widths, both big-endian in their payload and flagged by the
/// top bit of their first unit, and a zig-zag that is NOT protobuf's:
/// `(v & 1) ? -(v >> 1) : (v >> 1)`, so odd values are negative.
class BldCursor {
public:
    BldCursor(const std::uint8_t* data, std::size_t size)
        : data_(data), size_(size) {}

    std::uint8_t U8();
    std::uint16_t U16();
    std::uint32_t Varint8();   ///< 1 byte, or 15 bits over 2
    std::uint32_t Varint16();  ///< 2 bytes, or 31 bits over 4
    std::int32_t DeltaVarint16();  ///< zig-zagged Varint16

    void Skip(std::size_t count);
    std::size_t Position() const { return at_; }
    bool AtEnd() const { return at_ == size_; }

private:
    const std::uint8_t* data_;
    std::size_t size_;
    std::size_t at_ = 0;
};

}  // namespace sdl3cpp::fs2024
