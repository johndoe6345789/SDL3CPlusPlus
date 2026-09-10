#include "services/interfaces/workflow/quake3/q3_sky_texture_name.hpp"

#include "services/interfaces/workflow/rendering/bsp_geometry_lumps.hpp"

namespace sdl3cpp::services::impl {

std::string FindSkyTextureName(
    const std::shared_ptr<std::vector<uint8_t>>& bspData) {
    if (!bspData) {
        return {};
    }
    const BspGeometryLumps lumps = ReadBspGeometryLumps(*bspData);
    for (int i = 0; i < lumps.numTextures; ++i) {
        const std::string name(lumps.textures[i].name);
        if (name.find("skies/") != std::string::npos) {
            return name;
        }
    }
    return {};
}

}  // namespace sdl3cpp::services::impl
