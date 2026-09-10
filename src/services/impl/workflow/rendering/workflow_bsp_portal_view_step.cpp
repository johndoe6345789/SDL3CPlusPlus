#include "services/interfaces/workflow/rendering/workflow_bsp_portal_view_step.hpp"
#include "services/interfaces/workflow/rendering/bsp_portal_view_helpers.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

WorkflowBspPortalViewStep::WorkflowBspPortalViewStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBspPortalViewStep::GetPluginId() const {
    return "bsp.portal_view";
}

void WorkflowBspPortalViewStep::Execute(const WorkflowStepDefinition& step,
                                        WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    auto* pipeline =
        context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline_bsp", nullptr);
    auto* lmTex =
        context.Get<SDL_GPUTexture*>("bsp_lightmap_atlas_gpu", nullptr);
    auto* lmSamp =
        context.Get<SDL_GPUSampler*>("bsp_lightmap_atlas_sampler", nullptr);
    const auto* mapNodes = context.TryGet<nlohmann::json>("map.nodes");
    const auto* entities = context.TryGet<nlohmann::json>("bsp.entities");

    if (!device || !window || !pipeline || !lmTex || !lmSamp || !mapNodes ||
        !mapNodes->is_array() || mapNodes->empty() || !entities) {
        return;
    }

    glm::vec3 dest;
    if (!FindPortalDestination(*entities, dest)) return;
    dest += glm::vec3(0.0f, 1.4f, 0.0f);

    const PortalViewTargets targets =
        EnsurePortalViewTargets(device, window, context);
    if (!targets.colorTex || !targets.depthTex || !targets.sampler) return;

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) return;

    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = targets.colorTex;
    colorTarget.clear_color = {0.02f, 0.03f, 0.05f, 1.0f};
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo depthTarget = {};
    depthTarget.texture = targets.depthTex;
    depthTarget.clear_depth = 1.0f;
    depthTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    depthTarget.store_op = SDL_GPU_STOREOP_DONT_CARE;

    SDL_GPURenderPass* pass =
        SDL_BeginGPURenderPass(cmd, &colorTarget, 1, &depthTarget);
    if (!pass) {
        SDL_CancelGPUCommandBuffer(cmd);
        return;
    }

    rendering::VertexUniformData vu;
    rendering::FragmentUniformData fu;
    BuildPortalViewUniforms(context, dest, vu, fu);

    DrawPortalViewGeometry(pass, cmd, context, *mapNodes, pipeline, lmTex,
                           lmSamp, vu, fu);

    SDL_EndGPURenderPass(pass);
    SDL_SubmitGPUCommandBuffer(cmd);
}

}  // namespace sdl3cpp::services::impl
