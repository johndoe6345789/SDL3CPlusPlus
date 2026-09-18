#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::fs2024 {

/// A classic DDS texture's compressed blocks, every mip, as stored.
struct DdsBlocks {
    std::uint32_t width = 0, height = 0, mips = 1;
    bool alpha = false;              ///< BC3 (`DXT5`) rather than BC1
    std::uint32_t blockBytes = 8;    ///< per 4x4 block
    std::vector<std::uint8_t> data;  ///< top mip first, tightly packed
};

/// Reads a classic DDS file's blocks without decoding them: FS2024's
/// landmark colour maps are `DXT1` or `DXT5`, which the GPU samples
/// as-is. Mips the file is too short to hold are dropped. Throws on
/// any other format (its `BC5S` normal maps).
DdsBlocks ReadDdsBlocks(const std::string& path);

}  // namespace sdl3cpp::fs2024
