#include "services/interfaces/workflow/fs2024/data/texture/fs2024_dds_array.hpp"

#include "services/interfaces/workflow/fs2024/data/texture/fs2024_bc7_decode.hpp"
#include "services/interfaces/workflow/fs2024/data/texture/fs2024_dds_array_info.hpp"

#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace sdl3cpp::fs2024 {

int DdsArrayLayerCount(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("DDS array '" + path + "': cannot open");
    return static_cast<int>(ReadDdsArrayInfo(in, path).layers);
}

DdsImage ReadDdsArrayLayer(const std::string& path, int layer) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("DDS array '" + path + "': cannot open");
    const DdsArrayInfo info = ReadDdsArrayInfo(in, path);
    if (!info.IsBc7()) {
        throw std::runtime_error("DDS array '" + path + "': decodes BC7 only");
    }
    if (layer < 0 || layer >= static_cast<int>(info.layers)) {
        throw std::runtime_error("DDS array '" + path + "': no such layer");
    }
    const std::uint32_t blocksWide = (info.width + 3) / 4;
    const std::uint32_t blocksHigh = (info.height + 3) / 4;
    std::vector<std::uint8_t> blocks(static_cast<std::size_t>(blocksWide) *
                                     blocksHigh * kBc7BlockBytes);
    in.seekg(static_cast<std::streamoff>(kDx10HeaderSize +
                                        info.layerBytes * layer));
    in.read(reinterpret_cast<char*>(blocks.data()),
           static_cast<std::streamsize>(blocks.size()));
    if (!in) throw std::runtime_error("DDS array: short layer read");

    DdsImage image;
    image.width = static_cast<int>(info.width);
    image.height = static_cast<int>(info.height);
    image.rgba.resize(static_cast<std::size_t>(info.width) * info.height * 4);
    const int stride = image.width * 4;
    for (std::uint32_t by = 0; by < blocksHigh; ++by) {
        for (std::uint32_t bx = 0; bx < blocksWide; ++bx) {
            DecodeBc7Block(blocks.data() +
                              (by * blocksWide + bx) * kBc7BlockBytes,
                          image.rgba.data() + by * 4 * stride + bx * 16,
                          stride);
        }
    }
    return image;
}

}  // namespace sdl3cpp::fs2024
