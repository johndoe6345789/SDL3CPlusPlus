#include "services/interfaces/workflow/gta5/gta5_texture_upload.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

std::int64_t FindTexture(const Gta5Resource& res, std::int64_t dictionary,
                         std::uint32_t nameHash) {
    const std::int64_t hashes = res.Follow(dictionary + 0x20);
    const std::vector<std::int64_t> textures =
        res.PointerList(dictionary + 0x30);
    const std::size_t count =
        std::min<std::size_t>(res.U16(dictionary + 0x28), textures.size());
    for (std::size_t i = 0; hashes >= 0 && i < count; ++i) {
        if (res.U32(hashes + 4 * static_cast<std::int64_t>(i)) == nameHash) {
            return textures[i];
        }
    }
    return -1;
}

}  // namespace

Gta5TextureBlob ReadGta5DictionaryTexture(const Gta5Resource& ytd,
                                          std::uint32_t nameHash,
                                          std::int64_t dictionary) {
    Gta5TextureBlob blob;
    blob.hash = nameHash;
    if (dictionary < 0) return blob;
    const std::int64_t tex = FindTexture(ytd, dictionary, nameHash);
    if (tex < 0) return blob;
    const std::uint32_t w = ytd.U16(tex + 0x18);
    const std::uint32_t h = ytd.U16(tex + 0x1A);
    const Gta5TextureFormat format = Gta5TextureFormatFor(ytd.U8(tex + 0x1F));
    const std::int64_t pixels = ytd.Follow(tex + 0x38);
    // Block formats want whole blocks at mip 0.
    if (!w || !h || pixels < 0 || !format.bytes ||
        (format.compressed && (w % 4 || h % 4))) {
        return blob;
    }

    // Keep the mips that fit in the file; a truncated chain still draws.
    const std::uint32_t stored = std::max<std::uint32_t>(1, ytd.U8(tex + 0x22));
    const std::uint64_t room = ytd.data.size() - std::uint64_t(pixels);
    std::uint64_t total = 0;
    std::uint32_t levels = 0;
    for (; levels < stored; ++levels) {
        const std::uint64_t bytes = Gta5MipBytes(
            format, std::max(1u, w >> levels), std::max(1u, h >> levels));
        if (total + bytes > room) break;
        total += bytes;
    }
    if (levels == 0) return blob;

    blob.format = format;
    blob.width = w;
    blob.height = h;
    blob.levels = levels;
    blob.bytes.assign(ytd.data.begin() + pixels,
                      ytd.data.begin() + pixels + std::int64_t(total));
    return blob;
}

}  // namespace sdl3cpp::services::impl
