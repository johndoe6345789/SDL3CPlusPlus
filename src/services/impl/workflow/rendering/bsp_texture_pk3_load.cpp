#include "services/interfaces/workflow/rendering/bsp_texture_gpu_upload_internal.hpp"

#include <stb_image.h>
#include <zip.h>

#include <vector>

namespace sdl3cpp::services::impl {
namespace {

/// Tries one pk3 entry; returns a null-texture upload if it doesn't decode.
BspTextureUpload TryLoadEntry(zip_t* archive, SDL_GPUDevice* device,
                              const std::string& entryName) {
    zip_stat_t stat;
    if (zip_stat(archive, entryName.c_str(), 0, &stat) != 0) {
        return {};
    }
    zip_file_t* file = zip_fopen(archive, entryName.c_str(), 0);
    if (!file) {
        return {};
    }
    std::vector<uint8_t> data(stat.size);
    zip_fread(file, data.data(), stat.size);
    zip_fclose(file);

    int width = 0, height = 0, channels = 0;
    unsigned char* pixels =
        stbi_load_from_memory(data.data(), static_cast<int>(data.size()),
                              &width, &height, &channels, 4);
    if (!pixels) {
        return {};
    }
    BspTextureUpload upload =
        bsp_texture_detail::UploadRgba8WithMips(device, pixels, width, height);
    stbi_image_free(pixels);
    return upload;
}

}  // namespace

BspTextureUpload LoadBspTextureFromPk3(
    zip_t* archive, SDL_GPUDevice* device, const std::string& texName,
    const std::map<std::string, std::string>& shaderImages) {
    static const char* kExtensions[] = {".jpg", ".tga", ".png"};

    std::vector<std::string> bases{texName};
    const auto viaScript = shaderImages.find(texName);
    if (viaScript != shaderImages.end() && viaScript->second != texName) {
        bases.push_back(viaScript->second);
    }

    for (const std::string& base : bases) {
        for (const char* ext : kExtensions) {
            BspTextureUpload upload = TryLoadEntry(archive, device, base + ext);
            if (upload.texture) {
                upload.viaShader = (base != texName);
                return upload;
            }
        }
    }
    return {};
}

}  // namespace sdl3cpp::services::impl
