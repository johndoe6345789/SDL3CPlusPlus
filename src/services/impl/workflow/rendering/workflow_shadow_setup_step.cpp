#include "services/interfaces/workflow/rendering/workflow_shadow_setup_step.hpp"
#include "services/interfaces/workflow/rendering/shadow_map_resources.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowShadowSetupStep::WorkflowShadowSetupStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowShadowSetupStep::GetPluginId() const {
    return "shadow.setup";
}

void WorkflowShadowSetupStep::Execute(const WorkflowStepDefinition& step,
                                      WorkflowContext& context) {
    WorkflowStepParameterResolver params;

    auto getNum = [&](const char* name, float def) -> float {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number)
                   ? static_cast<float>(p->numberValue)
                   : def;
    };

    const int map_size = static_cast<int>(getNum("map_size", 2048));
    const float scene_extent = getNum("scene_extent", 15.0f);
    const float near_plane = getNum("near_plane", 0.1f);
    const float far_plane = getNum("far_plane", 50.0f);

    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) throw std::runtime_error("shadow.setup: GPU device not found");

    const ShadowDepthTarget target = CreateShadowDepthTarget(device, map_size);
    const glm::mat4 lightVP = ComputeShadowLightViewProjection(
        context, scene_extent, near_plane, far_plane);

    // Store depth resources and light matrix in context
    //    NOTE: The shadow *pipeline* is no longer created here.
    //    Use graphics.gpu.shader.compile + graphics.gpu.pipeline.create in
    //    JSON to build the shadow pipeline with depth_bias,
    //    num_color_targets=0, etc.
    context.Set<SDL_GPUTexture*>("shadow_depth_texture", target.texture);
    context.Set<SDL_GPUSampler*>("shadow_depth_sampler", target.sampler);

    // Store light VP as JSON array (16 floats)
    std::vector<float> vp_data(16);
    std::memcpy(vp_data.data(), glm::value_ptr(lightVP), sizeof(float) * 16);
    nlohmann::json shadow_state;
    shadow_state["light_vp"] = vp_data;
    shadow_state["map_size"] = map_size;
    context.Set("shadow.state", shadow_state);

    if (logger_) {
        logger_->Info(
            "shadow.setup: Created " + std::to_string(map_size) + "x" +
            std::to_string(map_size) +
            " shadow map + sampler (pipeline deferred to JSON)");
    }
}

}  // namespace sdl3cpp::services::impl
