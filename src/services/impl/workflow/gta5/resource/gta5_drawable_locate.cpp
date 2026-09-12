#include "services/interfaces/workflow/gta5/resource/gta5_drawable_mesh.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace sdl3cpp::services::impl {

std::int64_t LocateGta5Drawable(const Gta5Resource& res,
                                const std::string& path,
                                std::uint32_t hash) {
    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (ext == ".ydr") return 0;
    if (ext == ".yft") return res.Follow(0x30);
    if (ext != ".ydd") return -1;

    const std::int64_t hashes = res.Follow(0x20);
    const std::vector<std::int64_t> entries = res.PointerList(0x30);
    for (std::size_t i = 0; hashes >= 0 && i < entries.size(); ++i) {
        if (res.U32(hashes + 4 * static_cast<std::int64_t>(i)) == hash) {
            return entries[i];
        }
    }
    return -1;
}

}  // namespace sdl3cpp::services::impl
