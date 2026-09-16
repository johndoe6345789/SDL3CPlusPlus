#include "services/interfaces/workflow/gta5/render/gta5_draw_bind.hpp"

namespace sdl3cpp::services::impl {
namespace {

using Binding = SDL_GPUTextureSamplerBinding;

/// Layer `i`, or the diffuse where the submesh has none.
Binding LayerOr(const Gta5SubMesh& sub, int i, Binding diffuse) {
    return sub.layers[i] ? Binding{sub.layers[i], sub.layerSamplers[i]}
                         : diffuse;
}

}  // namespace

// The shaders read their own textures, then the sun's shadow map, then
// the water map; lit surfaces their normal and specular maps last.
int BindGta5SubMeshTextures(const Gta5DrawContext& draw,
                            const Gta5SubMesh& sub,
                            Gta5BoundTextures& bound) {
    const Binding diffuse = sub.texture
                                ? Binding{sub.texture, sub.sampler}
                                : Binding{draw.texture, draw.sampler};
    if (!diffuse.texture || !diffuse.sampler || !draw.shadowTexture ||
        !draw.shadowSampler || !draw.waterMap || !draw.waterSampler) {
        return -1;
    }
    const Binding shadow{draw.shadowTexture, draw.shadowSampler};
    const Binding water{draw.waterMap, draw.waterSampler};
    if (sub.terrain) {
        Binding layers[7];  // 4 layers, mask, shadow, water
        for (int i = 0; i < 5; ++i)
            layers[i] = LayerOr(sub, i, diffuse);
        layers[5] = shadow;
        layers[6] = water;
        SDL_BindGPUFragmentSamplers(draw.pass, 0, layers, 7);
        bound = Gta5BoundTextures{};
        return 1;
    }
    const Gta5BoundTextures wanted{diffuse.texture, sub.layers[0],
                                   sub.layers[1]};
    if (wanted.diffuse == bound.diffuse && wanted.bump == bound.bump &&
        wanted.specular == bound.specular) {
        return 0;
    }
    const Binding bindings[5] = {diffuse, shadow, water,
                                 LayerOr(sub, 0, diffuse),
                                 LayerOr(sub, 1, diffuse)};
    SDL_BindGPUFragmentSamplers(draw.pass, 0, bindings, 5);
    bound = wanted;
    return 1;
}

}  // namespace sdl3cpp::services::impl
