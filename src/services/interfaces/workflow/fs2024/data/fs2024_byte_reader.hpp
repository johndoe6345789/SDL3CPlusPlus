#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace sdl3cpp::fs2024 {

/// Reads a little-endian POD value out of `data` at byte offset `at`,
/// bounds-checked. Mirrors gta5's own resource-parsing style (plain
/// memcpy into a POD, not an overlaid packed struct).
template <typename T>
T ReadAt(const std::vector<std::uint8_t>& data, std::size_t at) {
    if (at + sizeof(T) > data.size()) {
        throw std::runtime_error("ReadAt: offset runs past the end of "
                                 "the buffer");
    }
    T value{};
    std::memcpy(&value, data.data() + at, sizeof(T));
    return value;
}

}  // namespace sdl3cpp::fs2024
