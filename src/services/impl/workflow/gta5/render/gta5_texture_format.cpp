#include "services/interfaces/workflow/gta5/render/gta5_texture_upload.hpp"

namespace sdl3cpp::services::impl {

Gta5TextureFormat Gta5TextureFormatFor(std::uint8_t g9) {
    switch (g9) {
        case 0x47:  // BC1_UNORM
        case 0x48:  // BC1_UNORM_SRGB
            return {SDL_GPU_TEXTUREFORMAT_BC1_RGBA_UNORM, 8, true};
        case 0x4A:
        case 0x4B:
            return {SDL_GPU_TEXTUREFORMAT_BC2_RGBA_UNORM, 16, true};
        case 0x4D:
        case 0x4E:
            return {SDL_GPU_TEXTUREFORMAT_BC3_RGBA_UNORM, 16, true};
        case 0x50:
            return {SDL_GPU_TEXTUREFORMAT_BC4_R_UNORM, 8, true};
        case 0x53:
            return {SDL_GPU_TEXTUREFORMAT_BC5_RG_UNORM, 16, true};
        case 0x62:
        case 0x63:
            return {SDL_GPU_TEXTUREFORMAT_BC7_RGBA_UNORM, 16, true};
        case 0x1C:
        case 0x1D:
            return {SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, 4, false};
        case 0x57:
        case 0x5B:
            return {SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM, 4, false};
        default:
            return {};
    }
}

std::uint64_t Gta5MipBytes(const Gta5TextureFormat& format,
                           std::uint32_t width, std::uint32_t height) {
    if (!format.compressed) {
        return std::uint64_t{width} * height * format.bytes;
    }
    // Whole 4x4 blocks, so a 2x2 mip still costs one block.
    return std::uint64_t{(width + 3) / 4} * ((height + 3) / 4) *
           format.bytes;
}

}  // namespace sdl3cpp::services::impl
