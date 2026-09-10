#include "services/interfaces/workflow/quake3/q3_hud_head_render.hpp"

namespace sdl3cpp::services::impl {

bool RenderHeadPortraitPass(SDL_GPUCommandBuffer* cmd,
                            const HeadRenderTargets& targets,
                            SDL_GPUGraphicsPipeline* pipeline,
                            const glm::mat4& mvp, const glm::mat4& model,
                            const rendering::FragmentUniformData& fu,
                            WorkflowContext& context, int size) {
    SDL_GPUColorTargetInfo cti{};
    cti.texture     = targets.color;
    cti.clear_color = {0.05f, 0.05f, 0.06f, 1.0f};  // near-black background
    cti.load_op     = SDL_GPU_LOADOP_CLEAR;
    cti.store_op    = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo dti{};
    dti.texture     = targets.depth;
    dti.clear_depth = 1.0f;
    dti.load_op     = SDL_GPU_LOADOP_CLEAR;
    dti.store_op    = SDL_GPU_STOREOP_DONT_CARE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &cti, 1, &dti);
    if (!pass) return false;

    SDL_GPUViewport vp{};
    vp.x         = 0;
    vp.y         = 0;
    vp.w         = static_cast<float>(size);
    vp.h         = static_cast<float>(size);
    vp.min_depth = 0.0f;
    vp.max_depth = 1.0f;
    SDL_SetGPUViewport(pass, &vp);

    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    DrawHeadMd3("head", mvp, model, fu, pass, cmd, context);

    SDL_EndGPURenderPass(pass);
    return true;
}

}  // namespace sdl3cpp::services::impl
