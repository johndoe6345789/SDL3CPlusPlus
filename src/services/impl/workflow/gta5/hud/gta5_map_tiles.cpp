#include "services/interfaces/workflow/gta5/hud/gta5_map_overlay.hpp"

#include "services/interfaces/workflow/gta5/render/gta5_texture_upload.hpp"
#include "services/interfaces/workflow/gta5/resource/gta5_resource.hpp"

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
    const Gta5TextureBlob blob =
        ReadGta5DictionaryTexture(res, Gta5Hash(name));
    return UploadGta5TextureBlob(blob, device, uploads).texture;
}

}  // namespace

bool LoadGta5MapTiles(Gta5MapOverlay& map, SDL_GPUDevice* device,
                      const std::string& dir, Gta5UploadBatch& uploads) {
    for (int i = 0; i < 6; ++i) {
        map.tiles[i] = LoadTile(device, dir, i / 2, i % 2, uploads);
        if (!map.tiles[i]) return false;
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
