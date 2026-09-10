#include "services/interfaces/workflow/rendering/overlay_sw_end_resources_internal.hpp"

#include <fstream>

namespace sdl3cpp::services::impl::overlay_sw_end_detail {

std::vector<uint8_t> LoadBinary(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return {};
    }
    const auto size = file.tellg();
    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

}  // namespace sdl3cpp::services::impl::overlay_sw_end_detail
