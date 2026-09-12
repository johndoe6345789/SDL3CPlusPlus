#include "services/interfaces/workflow/gta5/render/gta5_shadow_step.hpp"

#include "services/interfaces/workflow/gta5/stream/gta5_proxy.hpp"
#include "services/interfaces/workflow/gta5/render/gta5_shadow.hpp"
#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>

#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowGta5ShadowDrawStep::WorkflowGta5ShadowDrawStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5ShadowDrawStep::GetPluginId() const {
    return "gta5.shadow.draw";
}

void WorkflowGta5ShadowDrawStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    if (!state_ || context.GetBool("frame_skip", false)) return;
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        "gpu_pipeline_gta5_shadow", nullptr);
    const int size = static_cast<int>(Gta5NumberOr(step, "size", 4096.f));
    if (!device || !pipeline || !Ensure(device, size)) return;
    context.Set<SDL_GPUTexture*>("shadow_depth_texture", depth_);
    context.Set<SDL_GPUSampler*>("shadow_depth_sampler", sampler_);

    const auto light = context.Get<nlohmann::json>("lighting.directional",
                                                   nlohmann::json::object());
    const auto dir = light.value("direction", std::vector<float>{0, -1, 0});
    const auto camera =
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.f));
    const glm::mat4 lightVP = Gta5SunShadowViewProj(
        glm::vec3(dir[0], dir[1], dir[2]), camera,
        Gta5NumberOr(step, "radius", 150.f), size);
    context.Set<glm::mat4>("render.shadow_vp", lightVP);

    Gta5CullOptions options;
    options.sizeRatio = Gta5NumberOr(step, "cull_size_ratio", 0.01f);
    options.kinds = 1u | 1u << static_cast<int>(Gta5ProxyKind::Shadow);
    BuildGta5InstanceBatch(*state_, lightVP, camera, options, casters_);
    if (!UploadGta5InstanceBatch(device, casters_)) return;

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) return;
    SDL_GPUDepthStencilTargetInfo target = {};
    target.texture = depth_;
    target.clear_depth = 1.f;
    target.load_op = SDL_GPU_LOADOP_CLEAR;
    target.store_op = SDL_GPU_STOREOP_STORE;
    target.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    target.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    target.cycle = true;  // last frame's scene may still be reading it
    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, nullptr, 0, &target);
    if (pass) {
        SDL_BindGPUGraphicsPipeline(pass, pipeline);
        const int drawn = DrawGta5ShadowCasters(
            *state_, casters_, cmd, pass, lightVP,
            context.Get<SDL_GPUTexture*>("walls_texture_gpu", nullptr),
            context.Get<SDL_GPUSampler*>("walls_texture_sampler", nullptr));
        SDL_EndGPURenderPass(pass);
        if (logger_ && !logged_ && drawn > 0) {
            logged_ = true;
            logger_->Info("gta5.shadow.draw: " + std::to_string(drawn) +
                          " caster draws into a " + std::to_string(size) +
                          " map");
        }
    }
    SDL_SubmitGPUCommandBuffer(cmd);
}

}  // namespace sdl3cpp::services::impl
