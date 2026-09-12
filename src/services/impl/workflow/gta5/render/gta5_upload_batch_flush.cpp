#include "services/interfaces/workflow/gta5/render/gta5_upload_batch.hpp"

namespace sdl3cpp::services::impl {

void Gta5UploadBatch::Flush(SDL_GPUDevice* device) {
    if (mapped_ && device) SDL_UnmapGPUTransferBuffer(device, transfer_);
    mapped_ = nullptr;
    used_ = 0;
    SDL_GPUCommandBuffer* cmd =
        copies_.empty() || !device ? nullptr
                                   : SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) {
        copies_.clear();
        return;
    }
    SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(cmd);
    for (const Copy& c : copies_) {
        if (c.buffer) {
            const SDL_GPUTransferBufferLocation from = {transfer_, c.source};
            const SDL_GPUBufferRegion to = {c.buffer, c.target, c.size};
            SDL_UploadToGPUBuffer(pass, &from, &to, false);
            continue;
        }
        SDL_GPUTextureTransferInfo from = {};
        from.transfer_buffer = transfer_;
        from.offset = c.source;  // rows tightly packed
        SDL_GPUTextureRegion to = {};
        to.texture = c.texture;
        to.mip_level = c.target;
        to.w = c.width;
        to.h = c.height;
        to.d = 1;
        SDL_UploadToGPUTexture(pass, &from, &to, false);
    }
    SDL_EndGPUCopyPass(pass);
    SDL_SubmitGPUCommandBuffer(cmd);
    copies_.clear();
}

}  // namespace sdl3cpp::services::impl
