#include "services/interfaces/workflow/rendering/postfx_ssao_resources.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace sdl3cpp::services::impl {

SDL_GPUTexture* GetOrCreateSsaoTexture(WorkflowContext& context,
                                       SDL_GPUDevice* device, uint32_t width,
                                       uint32_t height) {
    auto* ssaoTex =
        context.Get<SDL_GPUTexture*>("postfx_ssao_texture", nullptr);
    const auto ssaoW = context.Get<uint32_t>("postfx_ssao_width", 0u);
    const auto ssaoH = context.Get<uint32_t>("postfx_ssao_height", 0u);

    if (ssaoTex && ssaoW == width && ssaoH == height) {
        return ssaoTex;
    }

    if (ssaoTex) SDL_ReleaseGPUTexture(device, ssaoTex);

    SDL_GPUTextureCreateInfo texInfo = {};
    texInfo.type                     = SDL_GPU_TEXTURETYPE_2D;
    texInfo.format                   = SDL_GPU_TEXTUREFORMAT_R8_UNORM;
    texInfo.width                    = width;
    texInfo.height                   = height;
    texInfo.layer_count_or_depth     = 1;
    texInfo.num_levels               = 1;
    texInfo.usage =
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;

    ssaoTex = SDL_CreateGPUTexture(device, &texInfo);
    context.Set<SDL_GPUTexture*>("postfx_ssao_texture", ssaoTex);
    context.Set<uint32_t>("postfx_ssao_width", width);
    context.Set<uint32_t>("postfx_ssao_height", height);
    return ssaoTex;
}

bool BuildSsaoUniforms(const glm::mat4& proj, uint32_t width, uint32_t height,
                       const std::vector<float>& kernel, SSAOUniformData& out) {
    if (kernel.size() < 64) return false;

    const glm::mat4 invProj = glm::inverse(proj);
    std::memcpy(out.projection, glm::value_ptr(proj), 64);
    std::memcpy(out.inv_projection, glm::value_ptr(invProj), 64);
    out.params[0] = 0.5f;    // radius
    out.params[1] = 0.025f;  // bias
    out.params[2] = 1.0f / float(width);
    out.params[3] = 1.0f / float(height);
    std::memcpy(out.kernel, kernel.data(), 16 * 4 * sizeof(float));
    return true;
}

}  // namespace sdl3cpp::services::impl
