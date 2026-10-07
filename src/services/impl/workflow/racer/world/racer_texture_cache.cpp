#include "services/interfaces/workflow/racer/world/racer_gpu_upload.hpp"

#include "services/interfaces/workflow/graphics/texture_gpu_upload.hpp"
#include "services/interfaces/workflow/graphics/texture_load_sampler.hpp"
#include "services/interfaces/workflow/racer/data/racer_upscale.hpp"

#include <cstdlib>
#include <cstring>
#include <exception>

namespace sdl3cpp::services::impl {
namespace {

std::uint64_t Key(const RacerMaterialRef& m) {
    return (static_cast<std::uint64_t>(m.textureIndex) << 40) ^
           (static_cast<std::uint64_t>(m.format) << 24) ^
           (static_cast<std::uint64_t>(m.width) << 12) ^
           static_cast<std::uint64_t>(m.height) ^
           (m.doubleWidth ? 1ull << 62 : 0) ^
           (m.doubleHeight ? 1ull << 61 : 0);
}

}  // namespace

RacerGpuTexture AcquireRacerTexture(SDL_GPUDevice* device,
                                    RacerWorldState& state,
                                    const RacerMaterialRef& material) {
    if (material.textureIndex < 0) return state.white;
    const std::uint64_t key = Key(material);
    const auto found = state.textures.find(key);
    if (found != state.textures.end()) return found->second;
    RacerGpuTexture& entry = state.textures[key];
    RacerTexture t = DecodeRacerMaterialTexture(state.library, material);
    if (t.rgba.empty() || !device) return entry;
    SolidifyRacerTexture(t);
    entry = UploadRacerTexture(device, t, state.textureScale);
    return entry;
}

RacerGpuTexture UploadRacerTexture(SDL_GPUDevice* device, RacerTexture t,
                                   int scale) {
    RacerGpuTexture entry;
    if (scale >= 2) {
        t.rgba = UpscaleRgba(t.rgba, t.width, t.height, scale);
        t.width *= scale;
        t.height *= scale;
    }
    // texture.load's uploader takes ownership of a malloc'd RGBA buffer
    // (it frees with stbi_image_free, which is free()).
    LoadedTextureImage image;
    image.width = t.width;
    image.height = t.height;
    image.pixels = static_cast<unsigned char*>(std::malloc(t.rgba.size()));
    if (!image.pixels) return entry;
    std::memcpy(image.pixels, t.rgba.data(), t.rgba.size());
    try {
        const UploadedTexture uploaded = UploadTextureImage(device, image);
        entry.texture = uploaded.texture;
        entry.sampler = CreateTextureLoadSampler(device, uploaded.texture,
                                                 uploaded.numLevels, 0.f);
    } catch (const std::exception&) {
        entry = RacerGpuTexture{};
    }
    return entry;
}

}  // namespace sdl3cpp::services::impl
