#include "services/interfaces/workflow/rendering/workflow_shadow_setup_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>
#include <cstring>
#include <cmath>

namespace sdl3cpp::services::impl {

WorkflowShadowSetupStep::WorkflowShadowSetupStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowShadowSetupStep::GetPluginId() const {
    return "shadow.setup";
}

void WorkflowShadowSetupStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepParameterResolver params;

    auto getNum = [&](const char* name, float def) -> float {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number) ? static_cast<float>(p->numberValue) : def;
    };

    int map_size = static_cast<int>(getNum("map_size", 2048));
    float scene_extent = getNum("scene_extent", 15.0f);
    float near_plane = getNum("near_plane", 0.1f);
    float far_plane = getNum("far_plane", 50.0f);

    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) throw std::runtime_error("shadow.setup: GPU device not found");

    // 1. Create shadow depth texture
    SDL_GPUTextureCreateInfo tex_info = {};
    tex_info.type = SDL_GPU_TEXTURETYPE_2D;
    tex_info.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    tex_info.width = map_size;
    tex_info.height = map_size;
    tex_info.layer_count_or_depth = 1;
    tex_info.num_levels = 1;
    tex_info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;

    SDL_GPUTexture* shadow_tex = SDL_CreateGPUTexture(device, &tex_info);
    if (!shadow_tex) throw std::runtime_error("shadow.setup: Failed to create depth texture");

    // 2. Create shadow sampler (nearest, clamp to edge)
    SDL_GPUSamplerCreateInfo samp_info = {};
    samp_info.min_filter = SDL_GPU_FILTER_NEAREST;
    samp_info.mag_filter = SDL_GPU_FILTER_NEAREST;
    samp_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samp_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samp_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    SDL_GPUSampler* shadow_sampler = SDL_CreateGPUSampler(device, &samp_info);
    if (!shadow_sampler) throw std::runtime_error("shadow.setup: Failed to create shadow sampler");

    // 3. Compute light view-projection matrix from directional light
    glm::vec3 lightDir(0.0f, -1.0f, 0.0f);
    const auto* lighting = context.TryGet<nlohmann::json>("lighting.directional");
    if (lighting && lighting->contains("direction")) {
        auto d = (*lighting)["direction"].get<std::vector<float>>();
        lightDir = glm::normalize(glm::vec3(d[0], d[1], d[2]));
    }

    glm::vec3 lightPos = -lightDir * 25.0f;
    glm::vec3 up = (std::abs(lightDir.y) > 0.99f) ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0, 0, 0), up);
    glm::mat4 lightProj = glm::ortho(-scene_extent, scene_extent, -scene_extent, scene_extent, near_plane, far_plane);
    glm::mat4 lightVP = lightProj * lightView;

    // 4. Store depth resources and light matrix in context
    //    NOTE: The shadow *pipeline* is no longer created here.
    //    Use graphics.gpu.shader.compile + graphics.gpu.pipeline.create in JSON
    //    to build the shadow pipeline with depth_bias, num_color_targets=0, etc.
    context.Set<SDL_GPUTexture*>("shadow_depth_texture", shadow_tex);
    context.Set<SDL_GPUSampler*>("shadow_depth_sampler", shadow_sampler);

    // Store light VP as JSON array (16 floats)
    std::vector<float> vp_data(16);
    std::memcpy(vp_data.data(), glm::value_ptr(lightVP), sizeof(float) * 16);
    nlohmann::json shadow_state;
    shadow_state["light_vp"] = vp_data;
    shadow_state["map_size"] = map_size;
    context.Set("shadow.state", shadow_state);

    if (logger_) {
        logger_->Info("shadow.setup: Created " + std::to_string(map_size) + "x" +
                     std::to_string(map_size) + " shadow map + sampler (pipeline deferred to JSON)");
    }
}

}  // namespace sdl3cpp::services::impl
