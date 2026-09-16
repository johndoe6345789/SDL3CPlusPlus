#include "services/interfaces/workflow/gta5/render/gta5_shader_textures.hpp"

namespace sdl3cpp::services::impl {

std::uint32_t ReadGta5BumpTexture(const Gta5Resource& res,
                                  std::int64_t shader) {
    return Gta5FirstShaderTexture(res, shader, {"bumptex", "bumpsampler"});
}

std::uint32_t ReadGta5SpecularTexture(const Gta5Resource& res,
                                      std::int64_t shader) {
    return Gta5FirstShaderTexture(res, shader,
                                  {"speculartex", "specsampler"});
}

bool ReadGta5TerrainLayers(const Gta5Resource& res, std::int64_t shader,
                           std::array<std::uint32_t, 5>& layers) {
    static const char* const kGen9[] = {
        "diffusetexture_layer0", "diffusetexture_layer1",
        "diffusetexture_layer2", "diffusetexture_layer3"};
    static const char* const kOld[] = {
        "texturesampler_layer0", "texturesampler_layer1",
        "texturesampler_layer2", "texturesampler_layer3"};
    int found = 0;
    for (int i = 0; i < 4; ++i) {
        layers[i] =
            Gta5FirstShaderTexture(res, shader, {kGen9[i], kOld[i]});
        if (layers[i]) ++found;
    }
    layers[4] = Gta5FirstShaderTexture(res, shader,
                                       {"lookuptexture", "lookupsampler"});
    return found >= 2;
}

}  // namespace sdl3cpp::services::impl
