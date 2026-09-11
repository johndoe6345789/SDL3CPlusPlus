#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// A decompressed RSC7 resource: system pages, then graphics pages.
///
/// Pointers inside carry a page tag in their top nibble, 5 for system and
/// 6 for graphics, and Resolve turns one into a flat offset or -1. Reads
/// are bounds-checked and give zero past the end, because a format
/// reader that is subtly wrong should produce nothing rather than crash.
struct Gta5Resource {
    std::vector<std::uint8_t> data;
    std::uint32_t version{0};
    std::uint32_t systemSize{0};

    std::int64_t Resolve(std::uint64_t pointer) const;
    /// Resolve the pointer stored at `at`.
    std::int64_t Follow(std::int64_t at) const { return Resolve(U64(at)); }
    /// RAGE's collection shape: a pointer, then a u16 count. Entries that
    /// do not resolve stay as -1, so indices still line up.
    std::vector<std::int64_t> PointerList(std::int64_t at) const;

    std::uint8_t U8(std::int64_t at) const;
    std::uint16_t U16(std::int64_t at) const;
    std::uint32_t U32(std::int64_t at) const;
    std::uint64_t U64(std::int64_t at) const;
    float F32(std::int64_t at) const;
    std::string String(std::int64_t at) const;
};

/// Read and inflate one RSC7 file. With systemOnly the inflate stops once
/// the system pages are out: that is where a dictionary keeps its name
/// table, and a texture dictionary's pixels are nearly all the rest.
bool LoadGta5Resource(const std::string& path, Gta5Resource& out,
                      bool systemOnly = false);

/// RAGE's name hash: Jenkins one-at-a-time over the lowercased text.
std::uint32_t Gta5Hash(const std::string& text);

}  // namespace sdl3cpp::services::impl
