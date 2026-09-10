#include "services/interfaces/workflow/rendering/workflow_shadow_pass_step.hpp"
#include "services/interfaces/workflow/rendering/shadow_pass_helpers.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

WorkflowShadowPassStep::WorkflowShadowPassStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowShadowPassStep::GetPluginId() const {
    return "shadow.pass";
}

void WorkflowShadowPassStep::Execute(const WorkflowStepDefinition& step,
                                     WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* shadow_tex =
        context.Get<SDL_GPUTexture*>("shadow_depth_texture", nullptr);
    auto* shadow_pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        "shadow_pipeline", nullptr);
    if (!device || !shadow_tex || !shadow_pipeline) return;

    const auto* shadow_state = context.TryGet<nlohmann::json>("shadow.state");
    if (!shadow_state || !shadow_state->contains("light_vp")) return;

    auto vp_data = (*shadow_state)["light_vp"].get<std::vector<float>>();
    const glm::mat4 lightVP = glm::make_mat4(vp_data.data());

    auto* vb = context.Get<SDL_GPUBuffer*>("plane_unit_vb", nullptr);
    auto* ib = context.Get<SDL_GPUBuffer*>("plane_unit_ib", nullptr);
    const auto* mesh_meta = context.TryGet<nlohmann::json>("plane_unit");
    if (!vb || !ib || !mesh_meta) return;
    const uint32_t index_count = (*mesh_meta)["index_count"];

    auto bodies =
        context.Get<nlohmann::json>("physics_bodies", nlohmann::json::array());
    if (bodies.empty()) return;

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) return;

    SDL_GPUDepthStencilTargetInfo ds_target = {};
    ds_target.texture = shadow_tex;
    ds_target.clear_depth = 1.0f;
    ds_target.load_op = SDL_GPU_LOADOP_CLEAR;
    ds_target.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, nullptr, 0,
                                                     &ds_target);
    if (!pass) {
        SDL_SubmitGPUCommandBuffer(cmd);
        return;
    }

    SDL_BindGPUGraphicsPipeline(pass, shadow_pipeline);

    SDL_GPUBufferBinding vb_bind = {};
    vb_bind.buffer = vb;
    SDL_BindGPUVertexBuffers(pass, 0, &vb_bind, 1);
    SDL_GPUBufferBinding ib_bind = {};
    ib_bind.buffer = ib;
    SDL_BindGPUIndexBuffer(pass, &ib_bind, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    // Render each body as a shadow-casting box using pre-computed transforms.
    const ShadowFaceRotations rotations = BuildShadowFaceRotations();
    for (const auto& nameVal : bodies) {
        DrawShadowCasterBody(pass, cmd, context, nameVal.get<std::string>(),
                            lightVP, rotations, index_count);
    }

    SDL_EndGPURenderPass(pass);
    SDL_SubmitGPUCommandBuffer(cmd);
}

}  // namespace sdl3cpp::services::impl
