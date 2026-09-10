#include "services/interfaces/workflow/quake3/q3_md3_surface_texture.hpp"

#include "services/interfaces/workflow/quake3/q3_md3_gpu_upload.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_shader_images.hpp"
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

/// Scripts key their shaders without an extension; MD3s write the shader
/// name with one ("models/powerups/health/yellow.tga"), so try both.
std::string ScriptImageFor(const std::string& shaderName,
                           const std::string& pk3Path,
                           WorkflowContext& context) {
    if (shaderName.empty()) {
        return {};
    }
    const auto& images = Q3Md3ShaderImages(pk3Path, context);
    auto found         = images.find(shaderName);
    if (found == images.end()) {
        const auto dot = shaderName.rfind('.');
        if (dot == std::string::npos) {
            return {};
        }
        found = images.find(shaderName.substr(0, dot));
    }
    return found == images.end() ? std::string{} : found->second;
}

}  // namespace

void UploadSurfaceTexture(SDL_GPUDevice* device, const Q3Md3Source& source,
                          const uint8_t* surfacePtr,
                          const q3::Md3Surface& surface,
                          const std::string& keyPrefix,
                          WorkflowContext& context) {
    const std::string surfaceName(surface.name, strnlen(surface.name, 64));
    const std::string shaderName = SurfaceShaderName(surfacePtr, surface);
    auto* texture                = q3::TryLoadMd3Texture(
        device, source.pk3Path,
        q3::BuildMd3TextureCandidates(
            q3::SkinTextureFor(source.skinMap, surfaceName), shaderName,
            source.skin, source.path));
    if (!texture) {
        // Nothing on disk matched the name, so the shader is script-only:
        // draw what its first stage draws.
        const std::string image =
            ScriptImageFor(shaderName, source.pk3Path, context);
        if (!image.empty()) {
            texture =
                q3::TryLoadMd3Texture(device, source.pk3Path,
                                      {image + ".tga", image + ".jpg", image});
        }
    }
    if (!texture) {
        const uint8_t grey[4] = {128, 128, 128, 255};
        texture               = q3::UploadMd3Texture(device, grey, 1, 1);
    }
    context.Set<SDL_GPUTexture*>(keyPrefix + "_tex", texture);
    context.Set<SDL_GPUSampler*>(keyPrefix + "_samp",
                                 q3::MakeMd3LinearSampler(device));
}

}  // namespace sdl3cpp::services::impl
