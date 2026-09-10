#include "services/interfaces/workflow/quake3/q3_md3_gpu_upload.hpp"
#include "services/interfaces/workflow/quake3/q3_pk3_reader.hpp"

#include <stb_image.h>

namespace sdl3cpp::q3 {

SDL_GPUTexture* TryLoadMd3Texture(SDL_GPUDevice* device, const std::string& pk3,
                                  const std::vector<std::string>& candidates) {
    for (const auto& entry : candidates) {
        const auto raw = ReadPk3Entry(pk3, entry);
        if (raw.empty()) {
            continue;
        }
        int width = 0, height = 0, channels = 0;
        unsigned char* pixels =
            stbi_load_from_memory(raw.data(), static_cast<int>(raw.size()),
                                  &width, &height, &channels, 4);
        if (!pixels) {
            continue;
        }
        auto* texture = UploadMd3Texture(device, pixels, width, height);
        stbi_image_free(pixels);
        if (texture) {
            return texture;
        }
    }
    return nullptr;
}

}  // namespace sdl3cpp::q3
