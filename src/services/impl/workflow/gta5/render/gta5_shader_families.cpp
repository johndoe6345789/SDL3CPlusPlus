#include "services/interfaces/workflow/gta5/render/gta5_shader_families.hpp"

#include "services/interfaces/workflow/gta5/resource/gta5_resource.hpp"

#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

const std::unordered_set<std::uint32_t>& Gta5PaintShaderHashes() {
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
const std::unordered_set<std::uint32_t>& Gta5EmissiveShaderHashes() {
    static const std::unordered_set<std::uint32_t> hashes = [] {
        std::unordered_set<std::uint32_t> out;
        for (const char* name :
             {"emissive", "emissive_alpha", "emissive_clip",
              "emissive_speclum", "emissive_tnt", "emissive_alpha_tnt",
              "emissivenight", "emissivenight_alpha",
              "emissivenight_geomnightonly", "emissivestrong",
              "emissivestrong_alpha"}) {
            out.insert(Gta5Hash(name));
            out.insert(Gta5Hash(std::string(name) + ".sps"));
        }
        return out;
    }();
    return hashes;
}

}  // namespace sdl3cpp::services::impl
