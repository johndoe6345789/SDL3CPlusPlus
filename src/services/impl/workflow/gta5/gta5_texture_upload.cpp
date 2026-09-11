#include "services/interfaces/workflow/gta5/gta5_texture_upload.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

std::int64_t FindTexture(const Gta5Resource& ytd, std::uint32_t nameHash) {
    const std::int64_t hashes = ytd.Follow(0x20);
    const std::vector<std::int64_t> textures = ytd.PointerList(0x30);
    const std::size_t count =
        std::min<std::size_t>(ytd.U16(0x28), textures.size());
    for (std::size_t i = 0; hashes >= 0 && i < count; ++i) {
        if (ytd.U32(hashes + 4 * static_cast<std::int64_t>(i)) == nameHash) {
            return textures[i];
        }
    }
    return -1;
}

}  // namespace

Gta5GpuTexture UploadGta5DictionaryTexture(const Gta5Resource& ytd,
                                           std::uint32_t nameHash,
                                           SDL_GPUDevice* device) {
    Gta5GpuTexture out;
    const std::int64_t tex = FindTexture(ytd, nameHash);
    if (tex < 0) return out;
    const std::uint32_t w = ytd.U16(tex + 0x18);
    const std::uint32_t h = ytd.U16(tex + 0x1A);
    const Gta5TextureFormat format = Gta5TextureFormatFor(ytd.U8(tex + 0x1F));
    const std::int64_t pixels = ytd.Follow(tex + 0x38);
    // Block formats want whole blocks at mip 0.
    if (!w || !h || pixels < 0 || !format.bytes ||
        (format.compressed && (w % 4 || h % 4)) ||
        !SDL_GPUTextureSupportsFormat(device, format.gpu,
                                      SDL_GPU_TEXTURETYPE_2D,
                                      SDL_GPU_TEXTUREUSAGE_SAMPLER)) {
        return out;
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
    if (levels == 0) return out;

    SDL_GPUTextureCreateInfo info{};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = format.gpu;
    info.width = w;
    info.height = h;
    info.layer_count_or_depth = 1;
    info.num_levels = levels;
    info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &info);
    if (!texture) return out;
    if (!CopyGta5Mips(device, texture, ytd.data.data() + pixels, total,
                      format, w, h, levels)) {
        SDL_ReleaseGPUTexture(device, texture);
        return out;
    }
    out.texture = texture;
    out.levels = levels;
    return out;
}

}  // namespace sdl3cpp::services::impl
