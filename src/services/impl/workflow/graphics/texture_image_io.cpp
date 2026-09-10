#include "services/interfaces/workflow/graphics/texture_image_io.hpp"

#include <stb_image.h>

#include <cstdlib>
#include <stdexcept>

namespace sdl3cpp::services::impl {

std::string ResolveTextureImagePath(const std::string& raw) {
    std::string resolved = raw;
    if (!resolved.empty() && resolved[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home) resolved = std::string(home) + resolved.substr(1);
    }
    return resolved;
}

LoadedTextureImage LoadTextureImagePixels(const std::string& path) {
    LoadedTextureImage image;
    image.pixels =
        stbi_load(path.c_str(), &image.width, &image.height, nullptr, 4);
    if (!image.pixels) {
        throw std::runtime_error("texture.load: Failed to load image: " + path +
                                 " (" + std::string(stbi_failure_reason()) +
                                 ")");
    }
    return image;
}

void FreeTextureImagePixels(LoadedTextureImage& image) {
    if (image.pixels) {
        stbi_image_free(image.pixels);
        image.pixels = nullptr;
    }
}

}  // namespace sdl3cpp::services::impl
