#include "services/interfaces/workflow/gta5/gta5_shader_textures.hpp"

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

/// Normal (_n) and specular (_s) maps, which are never the diffuse.
bool IsDetailMap(const std::string& name) {
    const std::size_t n = name.size();
    return n > 2 && name[n - 2] == '_' &&
           (name[n - 1] == 'n' || name[n - 1] == 's');
}

std::uint32_t FirstPlainTexture(const Gta5Resource& res, std::int64_t refs) {
    for (int i = 0; refs >= 0 && i < 8; ++i) {
        const std::int64_t entry = res.Follow(refs + 8 * i);
        if (entry < 0 || entry >= res.systemSize) continue;
        const std::int64_t at = res.Follow(entry + 0x28);
        if (at < 0 || at >= res.systemSize) continue;
        const std::string name = res.String(at);
        if (!name.empty() && !IsDetailMap(name)) return Gta5Hash(name);
    }
    return 0;
}

}  // namespace

std::uint32_t ReadGta5DiffuseTexture(const Gta5Resource& res,
                                     std::int64_t shader) {
    // Gen9 renamed the samplers: DiffuseSampler is DiffuseTex, and
    // terrain's TextureSampler_layer0 is DiffuseTexture_layer0. The old
    // names are kept in case a file still uses them.
    static const std::uint32_t kNames[] = {
        Gta5Hash("diffusetex"), Gta5Hash("diffusetexture_layer0"),
        Gta5Hash("diffusesampler"), Gta5Hash("texturesampler_layer0")};
    const std::int64_t refs = res.Follow(shader + 0x10);
    const std::int64_t infos = res.Follow(shader + 0x20);
    if (refs < 0 || infos < 0 || infos >= res.systemSize) {
        return FirstPlainTexture(res, refs);
    }
    const std::uint32_t textures = res.U8(infos + 1);
    const std::uint32_t params = res.U8(infos + 4);
    for (const std::uint32_t wanted : kNames) {
        for (std::uint32_t i = 0; i < params; ++i) {
            const std::int64_t at = infos + 8 + 8 * std::int64_t(i);
            const std::uint32_t data = res.U32(at + 4);
            if ((data & 3u) != 0 || res.U32(at) != wanted) continue;
            const std::uint32_t slot = (data >> 2) & 0xFFu;
            if (slot >= textures) continue;
            if (const std::uint32_t hash = TextureName(
                    res, res.Follow(refs + 8 * std::int64_t(slot)))) {
                return hash;
            }
        }
    }
    // Described, but with no diffuse: no texture beats a guess, which
    // drew normal maps as lavender bushes.
    return 0;
}

}  // namespace sdl3cpp::services::impl
