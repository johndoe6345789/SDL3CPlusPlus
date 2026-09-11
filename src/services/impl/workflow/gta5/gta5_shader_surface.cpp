#include "services/interfaces/workflow/gta5/gta5_shader_surface.hpp"

#include "services/interfaces/workflow/gta5/gta5_shader_textures.hpp"

#include <string>
#include <unordered_set>

namespace sdl3cpp::services::impl {
namespace {

constexpr std::uint8_t kAlphaBucket = 1;
constexpr std::uint8_t kDecalBucket = 2;
constexpr std::uint8_t kCutoutBucket = 3;

const std::unordered_set<std::uint32_t>& PaintHashes() {
    static const std::unordered_set<std::uint32_t> hashes = [] {
        std::vector<std::string> names{"vehicle_mesh"};
        for (int n = 1; n <= 9; ++n) {
            names.push_back("vehicle_paint" + std::to_string(n));
        }
        const std::size_t plain = names.size();
        for (std::size_t i = 0; i < plain; ++i) {
            names.push_back(names[i] + "_enveff");
        }
        std::unordered_set<std::uint32_t> out;
        for (const std::string& name : names) {
            out.insert(Gta5Hash(name));
            out.insert(Gta5Hash(name + ".sps"));
        }
        return out;
    }();
    return hashes;
}

/// GTA's emissive shaders, by name and by file name.
const std::unordered_set<std::uint32_t>& EmissiveHashes() {
    static const std::unordered_set<std::uint32_t> hashes = [] {
        std::unordered_set<std::uint32_t> out;
        for (const char* name :
             {"emissive", "emissive_alpha", "emissive_clip", "emissive_speclum",
              "emissive_tnt", "emissive_alpha_tnt", "emissivenight",
              "emissivenight_alpha", "emissivenight_geomnightonly",
              "emissivestrong", "emissivestrong_alpha"}) {
            out.insert(Gta5Hash(name));
            out.insert(Gta5Hash(std::string(name) + ".sps"));
        }
        return out;
    }();
    return hashes;
}

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
            s.paint = checked && PaintHashes().count(res.U32(shader)) > 0;
            s.cutout = checked && !s.paint && bucket == kCutoutBucket;
            s.blend = checked && !s.paint &&
                      (bucket == kAlphaBucket || bucket == kDecalBucket);
            s.terrain = ReadGta5TerrainLayers(res, shader, s.layers);
            s.emissive =
                checked && EmissiveHashes().count(res.U32(shader)) > 0;
        }
        out.push_back(s);
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
