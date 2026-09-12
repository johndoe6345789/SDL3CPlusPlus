#include "services/interfaces/workflow/gta5/hud/gta5_map_overlay.hpp"

#include "services/interfaces/workflow/gta5/hud/gta5_map_art.hpp"
#include "services/interfaces/workflow/gta5/resource/gta5_resource.hpp"
#include "services/interfaces/workflow/gta5/render/gta5_texture_upload.hpp"
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

bool LoadTiles(Gta5MapOverlay& map, SDL_GPUDevice* device,
               const std::string& dir, Gta5UploadBatch& uploads) {
    for (int i = 0; i < 6; ++i) {
        map.tiles[i] = LoadTile(device, dir, i / 2, i % 2, uploads);
        if (!map.tiles[i]) return false;
    }
    return true;
}

SDL_GPUGraphicsPipeline* Pipeline(SDL_GPUDevice* device,
                                  SDL_GPUTextureFormat format) {
    SDL_GPUShader* vertex = nullptr;
    SDL_GPUShader* fragment = nullptr;
    if (*LoadOverlayShaderPair(device, vertex, fragment) != '\0') {
        return nullptr;
    }
    SDL_GPUGraphicsPipeline* pipeline =
        CreateOverlayPipeline(device, format, vertex, fragment);
    SDL_ReleaseGPUShader(device, vertex);
    SDL_ReleaseGPUShader(device, fragment);
    return pipeline;
}

}  // namespace

bool LoadGta5MapOverlay(Gta5MapOverlay& map, SDL_GPUDevice* device,
                        SDL_GPUTextureFormat format, const std::string& dir,
                        const std::string& poiFile, Gta5UploadBatch& uploads,
                        const std::shared_ptr<ILogger>& logger) {
    map.tried = true;
    map.pipeline = Pipeline(device, format);
    if (!map.pipeline || !CreateGta5MapGpu(map, device) ||
        !CreateGta5MapMarkers(map, device, uploads)) {
        if (logger) logger->Warn("gta5.map: could not create the overlay");
        return false;
    }
    if (!LoadTiles(map, device, dir, uploads)) {
        if (logger) logger->Warn("gta5.map: no minimap tiles in " + dir);
        return false;
    }
    map.pois = LoadGta5MapPois(poiFile, logger);
    if (!CreateGta5MapAtlas(map, device, uploads)) {
        if (logger) logger->Warn("gta5.map: could not build the icons");
        return false;
    }
    uploads.Flush(device);  // ahead of the frame that first draws them
    map.ready = true;
    if (logger) logger->Info("gta5.map: minimap tiles loaded from " + dir);
    return true;
}

}  // namespace sdl3cpp::services::impl
