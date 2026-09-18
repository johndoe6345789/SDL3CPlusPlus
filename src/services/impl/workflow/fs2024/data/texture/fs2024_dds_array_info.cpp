#include "services/interfaces/workflow/fs2024/data/texture/fs2024_dds_array_info.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace sdl3cpp::fs2024 {
namespace {

constexpr std::uint32_t kBc1BlockBytes = 8;

}  // namespace

DdsArrayInfo ReadDdsArrayInfo(std::ifstream& in, const std::string& path) {
    std::vector<std::uint8_t> header(kDx10HeaderSize);
    in.read(reinterpret_cast<char*>(header.data()), kDx10HeaderSize);
    if (!in || std::memcmp(header.data(), "DDS ", 4) != 0 ||
        std::memcmp(header.data() + 84, "DX10", 4) != 0) {
        throw std::runtime_error("DDS array '" + path + "': not DX10");
    }
    DdsArrayInfo info;
    std::memcpy(&info.height, header.data() + 12, 4);
    std::memcpy(&info.width, header.data() + 16, 4);
    std::memcpy(&info.mips, header.data() + 28, 4);
    std::uint32_t format = 0;
    std::memcpy(&format, header.data() + 128, 4);
    std::memcpy(&info.layers, header.data() + 140, 4);
    info.dxgiFormat = format;
    if (format == kDxgiBc1Unorm || format == kDxgiBc1UnormSrgb) {
        info.blockBytes = kBc1BlockBytes;
    } else if (!info.IsBc7()) {
        throw std::runtime_error("DDS array '" + path + "': not BC1 or BC7");
    }
    for (std::uint32_t mip = 0; mip < std::max(info.mips, 1u); ++mip) {
        const std::uint32_t w = std::max(info.width >> mip, 1u);
        const std::uint32_t h = std::max(info.height >> mip, 1u);
        info.layerBytes += static_cast<std::size_t>((w + 3) / 4) *
                           ((h + 3) / 4) * info.blockBytes;
    }
    return info;
}

}  // namespace sdl3cpp::fs2024
