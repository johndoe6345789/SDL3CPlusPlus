#pragma once

#include "services/interfaces/workflow/fs2024/data/texture/fs2024_dds_array_info.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::fs2024 {

/// A whole DX10 texture array's compressed blocks, untouched: every
/// layer's mip chain back to back, exactly as the DDS stores them and
/// exactly as a GPU wants them -- nothing is decoded.
struct DdsArrayBlocks {
    DdsArrayInfo info;
    std::vector<std::uint8_t> data;  ///< info.layers * info.layerBytes
};

DdsArrayBlocks ReadDdsArrayBlocks(const std::string& path);

/// Bytes of one mip level of one layer.
std::size_t DdsMipBytes(const DdsArrayInfo& info, std::uint32_t mip);

}  // namespace sdl3cpp::fs2024
