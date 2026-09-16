#include "services/interfaces/workflow/gta5/render/gta5_shader_surface.hpp"

#include "services/interfaces/workflow/gta5/render/gta5_shader_families.hpp"
#include "services/interfaces/workflow/gta5/render/gta5_shader_textures.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr std::uint8_t kAlphaBucket  = 1;
constexpr std::uint8_t kDecalBucket  = 2;
constexpr std::uint8_t kCutoutBucket = 3;

}  // namespace

std::vector<Gta5ShaderSurface> ReadGta5ShaderSurfaces(
    const Gta5Resource& res, std::int64_t drawable) {
    std::vector<Gta5ShaderSurface> out;
    const std::int64_t group = res.Follow(drawable + 0x10);
    if (group < 0) return out;
    for (const std::int64_t shader : res.PointerList(group + 0x10)) {
        Gta5ShaderSurface s;
        if (shader >= 0) {
            const std::uint8_t bucket = res.U8(shader + 0x39);
            const bool checked =
                bucket < 8 &&
                res.U32(shader + 0x3C) == ((1u << bucket) | 0xFF00u);
            s.texture = ReadGta5DiffuseTexture(res, shader);
            s.paint   = checked &&
                      Gta5PaintShaderHashes().count(res.U32(shader)) > 0;
            s.cutout = checked && !s.paint && bucket == kCutoutBucket;
            s.blend  = checked && !s.paint &&
                      (bucket == kAlphaBucket || bucket == kDecalBucket);
            s.terrain = ReadGta5TerrainLayers(res, shader, s.layers);
            // Anything else carries its normal and specular maps in the
            // first two layers; see gta5_model.frag.
            if (!s.terrain) {
                s.layers = {ReadGta5BumpTexture(res, shader),
                            ReadGta5SpecularTexture(res, shader), 0, 0, 0};
            }
            s.emissive = checked && Gta5EmissiveShaderHashes().count(
                                        res.U32(shader)) > 0;
        }
        out.push_back(s);
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
