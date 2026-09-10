#include "services/interfaces/workflow/rendering/shadow_map_resources.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <stdexcept>
#include <vector>

namespace sdl3cpp::services::impl {

ShadowDepthTarget CreateShadowDepthTarget(SDL_GPUDevice* device, int map_size) {
    SDL_GPUTextureCreateInfo tex_info = {};
    tex_info.type                     = SDL_GPU_TEXTURETYPE_2D;
    tex_info.format                   = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    tex_info.width                    = map_size;
    tex_info.height                   = map_size;
    tex_info.layer_count_or_depth     = 1;
    tex_info.num_levels               = 1;
    tex_info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET |
                     SDL_GPU_TEXTUREUSAGE_SAMPLER;

    SDL_GPUTexture* shadow_tex = SDL_CreateGPUTexture(device, &tex_info);
    if (!shadow_tex) {
        throw std::runtime_error(
            "shadow.setup: Failed to create depth texture");
    }

    SDL_GPUSamplerCreateInfo samp_info = {};
    samp_info.min_filter               = SDL_GPU_FILTER_NEAREST;
    samp_info.mag_filter               = SDL_GPU_FILTER_NEAREST;
    samp_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samp_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samp_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    SDL_GPUSampler* shadow_sampler = SDL_CreateGPUSampler(device, &samp_info);
    if (!shadow_sampler) {
        throw std::runtime_error(
            "shadow.setup: Failed to create shadow sampler");
    }

    return ShadowDepthTarget{shadow_tex, shadow_sampler};
}

glm::mat4 ComputeShadowLightViewProjection(const WorkflowContext& context,
                                           float scene_extent, float near_plane,
                                           float far_plane) {
    glm::vec3 lightDir(0.0f, -1.0f, 0.0f);
    const auto* lighting =
        context.TryGet<nlohmann::json>("lighting.directional");
    if (lighting && lighting->contains("direction")) {
        auto d   = (*lighting)["direction"].get<std::vector<float>>();
        lightDir = glm::normalize(glm::vec3(d[0], d[1], d[2]));
    }

    const glm::vec3 lightPos = -lightDir * 25.0f;
    const glm::vec3 up = (std::abs(lightDir.y) > 0.99f) ? glm::vec3(0, 0, 1)
                                                        : glm::vec3(0, 1, 0);
    const glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0, 0, 0), up);
    const glm::mat4 lightProj =
        glm::ortho(-scene_extent, scene_extent, -scene_extent, scene_extent,
                   near_plane, far_plane);
    return lightProj * lightView;
}

}  // namespace sdl3cpp::services::impl
