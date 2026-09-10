#include "services/interfaces/workflow/graphics/graphics_gpu_buffer_create.hpp"

#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

UploadedGpuBuffers CreateGpuVertexIndexBuffers(SDL_GPUDevice* device,
                                               uint32_t vertexSize,
                                               uint32_t indexSize) {
    SDL_GPUBufferCreateInfo vbufInfo = {};
    vbufInfo.usage                   = SDL_GPU_BUFFERUSAGE_VERTEX;
    vbufInfo.size                    = vertexSize;
    SDL_GPUBuffer* vbuf              = SDL_CreateGPUBuffer(device, &vbufInfo);
    if (!vbuf) {
        throw std::runtime_error(
            "graphics.buffer.upload: Failed to create vertex buffer: " +
            std::string(SDL_GetError()));
    }

    SDL_GPUBufferCreateInfo ibufInfo = {};
    ibufInfo.usage                   = SDL_GPU_BUFFERUSAGE_INDEX;
    ibufInfo.size                    = indexSize;
    SDL_GPUBuffer* ibuf              = SDL_CreateGPUBuffer(device, &ibufInfo);
    if (!ibuf) {
        SDL_ReleaseGPUBuffer(device, vbuf);
        throw std::runtime_error(
            "graphics.buffer.upload: Failed to create index buffer: " +
            std::string(SDL_GetError()));
    }

    return UploadedGpuBuffers{vbuf, ibuf};
}

}  // namespace sdl3cpp::services::impl
