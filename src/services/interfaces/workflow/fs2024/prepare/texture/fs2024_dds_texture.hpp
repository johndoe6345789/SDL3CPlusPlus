#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::tools::fs2024 {

/// A decoded DDS texture, top mip only, RGBA8 row-major.
struct DdsImage {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba;  ///< width*height*4 bytes
};

/// Decodes a classic (non-DX10-header) DDS file's top mip level.
/// FS2024's own textures seen so far are all BC1 (`DXT1`, opaque) or
/// BC3 (`DXT5`, alpha) -- both used for colour maps. Normal/roughness
/// maps use BC5 and are not decoded: this engine's fs2024 shader has
/// no normal mapping to feed them to.
DdsImage DecodeDds(const std::string& path);

}  // namespace sdl3cpp::tools::fs2024
