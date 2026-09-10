#include "services/interfaces/workflow/rendering/overlay_head_portrait.hpp"

#include <cstring>

namespace sdl3cpp::services::impl {
namespace {

bool EnsureHeadSampler(OverlaySwEndResources& res) {
    if (res.headSampler) {
        return true;
    }
    SDL_GPUSamplerCreateInfo sci = {};
    sci.min_filter               = SDL_GPU_FILTER_LINEAR;
    sci.mag_filter               = SDL_GPU_FILTER_LINEAR;
    sci.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sci.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sci.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    res.headSampler              = SDL_CreateGPUSampler(res.device, &sci);
    return res.headSampler != nullptr;
}

/// Uploads a 6-vertex quad for the face rect, in overlay-pixel NDC.
bool UploadHeadQuad(SDL_GPUCommandBuffer* cmd, OverlaySwEndResources& res,
                    float faceRectX, float faceRectY, float faceRectW,
                    float faceRectH, int overlayWidth, int overlayHeight) {
    const float ow = static_cast<float>(overlayWidth);
    const float oh = static_cast<float>(overlayHeight);
    const float x0 = 2.0f * faceRectX / ow - 1.0f;
    const float x1 = 2.0f * (faceRectX + faceRectW) / ow - 1.0f;
    const float y1 = 1.0f - 2.0f * faceRectY / oh;                // top
    const float y0 = 1.0f - 2.0f * (faceRectY + faceRectH) / oh;  // bottom

    const float verts[6][5] = {
        {x0, y1, 0.0f, 0.0f, 0.0f}, {x1, y1, 0.0f, 1.0f, 0.0f},
        {x1, y0, 0.0f, 1.0f, 1.0f}, {x0, y1, 0.0f, 0.0f, 0.0f},
        {x1, y0, 0.0f, 1.0f, 1.0f}, {x0, y0, 0.0f, 0.0f, 1.0f},
    };
    const uint32_t size = sizeof(verts);

    if (!res.headVertices) {
        SDL_GPUBufferCreateInfo bci = {};
        bci.usage                   = SDL_GPU_BUFFERUSAGE_VERTEX;
        bci.size                    = size;
        res.headVertices            = SDL_CreateGPUBuffer(res.device, &bci);
        if (!res.headVertices) {
            return false;
        }
    }

    SDL_GPUTransferBufferCreateInfo tbci = {};
    tbci.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size                            = size;
    SDL_GPUTransferBuffer* staging =
        SDL_CreateGPUTransferBuffer(res.device, &tbci);
    if (!staging) {
        return false;
    }
    if (void* mapped = SDL_MapGPUTransferBuffer(res.device, staging, false)) {
        std::memcpy(mapped, verts, size);
        SDL_UnmapGPUTransferBuffer(res.device, staging);
    }
    if (SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd)) {
        SDL_GPUTransferBufferLocation src = {staging, 0};
        SDL_GPUBufferRegion dst           = {res.headVertices, 0, size};
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);
    }
    SDL_ReleaseGPUTransferBuffer(res.device, staging);
    return true;
}

}  // namespace

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
