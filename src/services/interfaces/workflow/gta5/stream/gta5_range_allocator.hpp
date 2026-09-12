#pragma once

#include <cstdint>
#include <map>

namespace sdl3cpp::services::impl {

/// Hands out runs of a fixed-size space -- vertices or indices of one
/// arena block -- and takes them back, merging neighbours so the space
/// does not splinter as districts stream in and out.
class Gta5RangeAllocator {
public:
    explicit Gta5RangeAllocator(std::uint32_t capacity = 0);

    /// First fit. False when no free run is long enough.
    bool Allocate(std::uint32_t size, std::uint32_t& offset);
    void Free(std::uint32_t offset, std::uint32_t size);

private:
    std::map<std::uint32_t, std::uint32_t> free_;  // offset -> length
};

}  // namespace sdl3cpp::services::impl
