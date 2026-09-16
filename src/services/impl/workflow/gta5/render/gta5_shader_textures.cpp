#include "services/interfaces/workflow/gta5/render/gta5_shader_textures.hpp"

#include <initializer_list>
#include <string>

namespace sdl3cpp::services::impl {
namespace {

std::uint32_t TextureName(const Gta5Resource& res, std::int64_t texture) {
    if (texture < 0 || texture >= res.systemSize) return 0;
    const std::int64_t at = res.Follow(texture + 0x28);
    if (at < 0 || at >= res.systemSize) return 0;
    const std::string name = res.String(at);
    return name.empty() ? 0 : Gta5Hash(name);
}

/// The texture bound to the parameter named `wanted`, or 0.
std::uint32_t TextureParam(const Gta5Resource& res, std::int64_t shader,
                           std::uint32_t wanted) {
    const std::int64_t refs  = res.Follow(shader + 0x10);
    const std::int64_t infos = res.Follow(shader + 0x20);
    if (refs < 0 || infos < 0 || infos >= res.systemSize) return 0;
    const std::uint32_t textures = res.U8(infos + 1);
    const std::uint32_t params   = res.U8(infos + 4);
    for (std::uint32_t i = 0; i < params; ++i) {
        const std::int64_t at    = infos + 8 + 8 * std::int64_t(i);
        const std::uint32_t data = res.U32(at + 4);
        if ((data & 3u) != 0 || res.U32(at) != wanted) continue;
        const std::uint32_t slot = (data >> 2) & 0xFFu;
        if (slot < textures) {
            return TextureName(res,
                               res.Follow(refs + 8 * std::int64_t(slot)));
        }
    }
    return 0;
}

}  // namespace

std::uint32_t Gta5FirstShaderTexture(
    const Gta5Resource& res, std::int64_t shader,
    std::initializer_list<const char*> names) {
    for (const char* name : names) {
        const std::uint32_t hash =
            TextureParam(res, shader, Gta5Hash(name));
        if (hash) return hash;
    }
    return 0;
}

std::uint32_t ReadGta5DiffuseTexture(const Gta5Resource& res,
                                     std::int64_t shader) {
    // The old names too, in case a file still uses them.
    return Gta5FirstShaderTexture(
        res, shader,
        {"diffusetex", "diffusetexture_layer0", "diffusesampler",
         "texturesampler_layer0"});
}

}  // namespace sdl3cpp::services::impl
