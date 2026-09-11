#include "services/interfaces/workflow/gta5/gta5_shader_surface.hpp"

#include <string>
#include <unordered_set>

namespace sdl3cpp::services::impl {
namespace {

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

/// Normal (_n) and specular (_s) maps, which are never the diffuse.
bool IsDetailMap(const std::string& name) {
    const std::size_t n = name.size();
    return n > 2 && name[n - 2] == '_' &&
           (name[n - 1] == 'n' || name[n - 1] == 's');
}

/// The first texture reference that is not a detail map. G9 shaders keep
/// up to eight at +0x10, each with its name at +0x28.
std::uint32_t DiffuseTexture(const Gta5Resource& res, std::int64_t shader) {
    const std::int64_t refs = res.Follow(shader + 0x10);
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
            s.texture = DiffuseTexture(res, shader);
            s.paint = checked && PaintHashes().count(res.U32(shader)) > 0;
            s.cutout = checked && !s.paint && bucket == kCutoutBucket;
        }
        out.push_back(s);
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
