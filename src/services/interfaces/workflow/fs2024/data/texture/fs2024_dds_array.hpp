#pragma once

#include "services/interfaces/workflow/fs2024/data/texture/fs2024_dds_texture.hpp"

#include <cstdint>
#include <string>

namespace sdl3cpp::fs2024 {

/// One layer's top mip out of a DX10-header BC7 texture array --
/// how FS2024's building generator ships every facade, roof, window
/// and door texture it owns (bf-pgg/PGG/textures/TEXTURES_*.DDS.DDS,
/// 1888 layers of 256x256 in the albedo array alone). The layer index
/// is an asset's `index_albedo` (etc.) from the same folder's
/// textures.json.
DdsImage ReadDdsArrayLayer(const std::string& path, int layer);

/// How many layers that array holds.
int DdsArrayLayerCount(const std::string& path);

}  // namespace sdl3cpp::fs2024
