#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace sdl3cpp::services::impl {

/// Bounds-checked big-endian reads over one block item. Every Episode I
/// Racer block is stored big-endian (the PC port byte-swaps on load).
/// Reads past the end return zero rather than throwing, so a damaged
/// item yields an empty mesh instead of a crash.
class RacerBigEndianReader {
public:
    explicit RacerBigEndianReader(const std::vector<std::uint8_t>& data)
        : data_(data) {}

    std::size_t Size() const { return data_.size(); }

    bool Has(std::size_t offset, std::size_t bytes) const {
        return offset <= data_.size() && bytes <= data_.size() - offset;
    }

    std::uint8_t U8(std::size_t offset) const {
        return Has(offset, 1) ? data_[offset] : 0;
    }

    std::uint16_t U16(std::size_t offset) const {
        if (!Has(offset, 2)) return 0;
        return static_cast<std::uint16_t>((data_[offset] << 8) |
                                          data_[offset + 1]);
    }

    std::int16_t I16(std::size_t offset) const {
        return static_cast<std::int16_t>(U16(offset));
    }

    std::uint32_t U32(std::size_t offset) const {
        if (!Has(offset, 4)) return 0;
        return (static_cast<std::uint32_t>(data_[offset]) << 24) |
               (static_cast<std::uint32_t>(data_[offset + 1]) << 16) |
               (static_cast<std::uint32_t>(data_[offset + 2]) << 8) |
               static_cast<std::uint32_t>(data_[offset + 3]);
    }

    std::int32_t I32(std::size_t offset) const {
        return static_cast<std::int32_t>(U32(offset));
    }

    float F32(std::size_t offset) const {
        const std::uint32_t bits = U32(offset);
        float value = 0.0f;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

private:
    const std::vector<std::uint8_t>& data_;
};

}  // namespace sdl3cpp::services::impl
