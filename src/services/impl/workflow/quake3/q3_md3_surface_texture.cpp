#include "services/interfaces/workflow/quake3/q3_md3_surface_texture.hpp"

#include "services/interfaces/workflow/quake3/q3_md3_texture_candidates.hpp"

#include <cstring>

namespace sdl3cpp::services::impl {
namespace {

std::string SurfaceShaderName(const uint8_t* surfacePtr,
                              const q3::Md3Surface& surface) {
    if (surface.numShaders <= 0) {
        return {};
    }
    const auto* shaders =
        reinterpret_cast<const q3::Md3Shader*>(surfacePtr + surface.ofsShaders);
    return std::string(shaders[0].name, strnlen(shaders[0].name, 64));
}

}  // namespace

void UploadSurfaceTexture(SDL_GPUDevice* device, const Q3Md3Source& source,
                          const uint8_t* surfacePtr,
                          const q3::Md3Surface& surface,
                          const std::string& keyPrefix,
                          WorkflowContext& context) {
    const std::string surfaceName(surface.name, strnlen(surface.name, 64));
    auto* texture = q3::TryLoadMd3Texture(
        device, source.pk3Path,
        q3::BuildMd3TextureCandidates(
            q3::SkinTextureFor(source.skinMap, surfaceName),
            SurfaceShaderName(surfacePtr, surface), source.skin, source.path));
    if (!texture) {
        const uint8_t grey[4] = {128, 128, 128, 255};
        texture               = q3::UploadMd3Texture(device, grey, 1, 1);
    }
    context.Set<SDL_GPUTexture*>(keyPrefix + "_tex", texture);
    context.Set<SDL_GPUSampler*>(keyPrefix + "_samp",
                                 q3::MakeMd3LinearSampler(device));
}

}  // namespace sdl3cpp::services::impl
