#include "services/interfaces/workflow/fs2024/data/texture/fs2024_dds_array_blocks.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace sdl3cpp::fs2024 {

DdsArrayBlocks ReadDdsArrayBlocks(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("DDS array '" + path + "': cannot open");
    DdsArrayBlocks blocks;
    blocks.info = ReadDdsArrayInfo(in, path);
    blocks.data.resize(blocks.info.layerBytes * blocks.info.layers);
    in.read(reinterpret_cast<char*>(blocks.data.data()),
           static_cast<std::streamsize>(blocks.data.size()));
    if (!in) throw std::runtime_error("DDS array '" + path + "': short read");
    return blocks;
}

std::size_t DdsMipBytes(const DdsArrayInfo& info, std::uint32_t mip) {
    const std::uint32_t w = std::max(info.width >> mip, 1u);
    const std::uint32_t h = std::max(info.height >> mip, 1u);
    return static_cast<std::size_t>((w + 3) / 4) * ((h + 3) / 4) *
           info.blockBytes;
}

}  // namespace sdl3cpp::fs2024
