#include "services/interfaces/workflow/rendering/overlay_head_portrait.hpp"
#include "services/interfaces/workflow/rendering/overlay_head_quad_upload.hpp"
#include "services/interfaces/workflow/rendering/overlay_head_sampler.hpp"

namespace sdl3cpp::services::impl {

bool BlitHeadPortrait(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* swapchain,
                      OverlaySwEndResources& res, SDL_GPUTexture* headTexture,
                      float faceRectX, float faceRectY, float faceRectW,
                      float faceRectH, int overlayWidth, int overlayHeight) {
    if (!res.pipeline || !headTexture) {
        return false;
    }
    if (!EnsureHeadSampler(res)) {
        return false;
    }
    if (!UploadHeadQuad(cmd, res, faceRectX, faceRectY, faceRectW, faceRectH,
                        overlayWidth, overlayHeight)) {
        return false;
    }

    SDL_GPUColorTargetInfo target = {};
    target.texture                = swapchain;
    target.load_op                = SDL_GPU_LOADOP_LOAD;
    target.store_op               = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &target, 1, nullptr);
    if (!pass) {
        return false;
    }

    SDL_BindGPUGraphicsPipeline(pass, res.pipeline);
    SDL_GPUBufferBinding vb = {res.headVertices, 0};
    SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
    SDL_GPUTextureSamplerBinding ts = {headTexture, res.headSampler};
    SDL_BindGPUFragmentSamplers(pass, 0, &ts, 1);
    SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
    return true;
}

}  // namespace sdl3cpp::services::impl
