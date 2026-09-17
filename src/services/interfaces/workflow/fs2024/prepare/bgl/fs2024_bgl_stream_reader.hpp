#pragma once

#include <fstream>
#include <stdexcept>

namespace sdl3cpp::tools::fs2024 {

/// Seeks to `at` and reads one little-endian POD, for BGL files too
/// large to load whole (FS2024's own worldwide model library runs
/// past a gigabyte). Mirrors `ReadAt`'s bounds-checked style, just
/// against a stream instead of an in-memory buffer.
template <typename T>
T ReadPodAt(std::ifstream& in, std::uint64_t at) {
    in.seekg(static_cast<std::streamoff>(at));
    T value{};
    in.read(reinterpret_cast<char*>(&value), sizeof(T));
    if (!in) {
        throw std::runtime_error("BGL stream: short read at offset");
    }
    return value;
}

}  // namespace sdl3cpp::tools::fs2024
