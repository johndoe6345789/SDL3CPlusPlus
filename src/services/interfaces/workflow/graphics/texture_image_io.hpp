#pragma once

#include <string>

namespace sdl3cpp::services::impl {

/// Expands a leading `~` in `raw` to `$HOME`, exactly as texture.load
/// always has (a no-op when `HOME` isn't set).
std::string ResolveTextureImagePath(const std::string& raw);

/// One image decoded by stb_image, forced to 4 (RGBA) channels.
struct LoadedTextureImage {
    unsigned char* pixels = nullptr;
    int width             = 0;
    int height            = 0;
};

/// Decodes `path` with stb_image; throws std::runtime_error (prefixed
/// "texture.load: ...", including stb's failure reason) if it can't.
LoadedTextureImage LoadTextureImagePixels(const std::string& path);

/// Frees `image.pixels` (a no-op if already null) and nulls it out.
void FreeTextureImagePixels(LoadedTextureImage& image);

}  // namespace sdl3cpp::services::impl
