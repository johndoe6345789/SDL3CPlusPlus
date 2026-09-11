#include "services/interfaces/workflow/gta5/gta5_map_overlay.hpp"

#include "services/interfaces/workflow/gta5/gta5_resource.hpp"
#include "services/interfaces/workflow/gta5/gta5_texture_upload.hpp"
#include "services/interfaces/workflow/rendering/gpu_text_overlay_pipeline_factory.hpp"
#include "services/interfaces/workflow/rendering/gpu_text_overlay_shader_loader.hpp"

#include <cstdio>

namespace sdl3cpp::services::impl {
namespace {

SDL_GPUTexture* LoadTile(SDL_GPUDevice* device, const std::string& dir,
                         int row, int column, Gta5UploadBatch& uploads) {
    char name[32];
    std::snprintf(name, sizeof(name), "minimap_%d_%d", row, column);
    Gta5Resource res;
    if (!LoadGta5Resource(dir + "/" + name + ".ytd", res, false)) {
        return nullptr;
    }
    const Gta5TextureBlob blob = ReadGta5DictionaryTexture(res, Gta5Hash(name));
    return UploadGta5TextureBlob(blob, device, uploads).texture;
}

bool CreateBuffers(Gta5MapOverlay& map, SDL_GPUDevice* device) {
    const auto bytes =
        static_cast<Uint32>(kGta5MapQuads * 6 * 5 * sizeof(float));
    SDL_GPUBufferCreateInfo buffer = {};
    buffer.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    buffer.size = bytes;
    map.vertices = SDL_CreateGPUBuffer(device, &buffer);
    SDL_GPUTransferBufferCreateInfo staging = {};
    staging.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    staging.size = bytes;
    map.staging = SDL_CreateGPUTransferBuffer(device, &staging);
    SDL_GPUSamplerCreateInfo sampler = {};
    sampler.min_filter = SDL_GPU_FILTER_LINEAR;
    sampler.mag_filter = SDL_GPU_FILTER_LINEAR;
    sampler.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    map.sampler = SDL_CreateGPUSampler(device, &sampler);
    return map.vertices && map.staging && map.sampler;
}

}  // namespace

bool LoadGta5MapOverlay(Gta5MapOverlay& map, SDL_GPUDevice* device,
                        SDL_GPUTextureFormat format, const std::string& dir,
                        Gta5UploadBatch& uploads,
                        const std::shared_ptr<ILogger>& logger) {
    map.tried = true;
    SDL_GPUShader* vertex = nullptr;
    SDL_GPUShader* fragment = nullptr;
    const char* err = LoadOverlayShaderPair(device, vertex, fragment);
    if (*err == '\0') {
        map.pipeline = CreateOverlayPipeline(device, format, vertex, fragment);
        SDL_ReleaseGPUShader(device, vertex);
        SDL_ReleaseGPUShader(device, fragment);
    }
    if (!map.pipeline || !CreateBuffers(map, device) ||
        !CreateGta5MapMarkers(map, device, uploads)) {
        if (logger) logger->Warn("gta5.map: could not create the overlay");
        return false;
    }
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 2; ++column) {
            SDL_GPUTexture*& tile = map.tiles[row * 2 + column];
            tile = LoadTile(device, dir, row, column, uploads);
            if (tile) continue;
            if (logger) logger->Warn("gta5.map: no minimap tiles in " + dir);
            return false;
        }
    }
    uploads.Flush(device);  // ahead of the frame that first draws them
    map.ready = true;
    if (logger) logger->Info("gta5.map: minimap tiles loaded from " + dir);
    return true;
}

}  // namespace sdl3cpp::services::impl
