#include "services/interfaces/workflow/gta5/gta5_map_art.hpp"

#include <cstdint>

namespace sdl3cpp::services::impl {

SDL_GPUTexture* CreateGta5MapRgba(SDL_GPUDevice* device, int width,
                                  int height,
                                  const std::vector<std::uint8_t>& pixels,
                                  Gta5UploadBatch& uploads) {
    SDL_GPUTextureCreateInfo info = {};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    info.width = static_cast<Uint32>(width);
    info.height = static_cast<Uint32>(height);
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &info);
    if (texture &&
        !uploads.StageTexture(device, pixels.data(),
                              static_cast<std::uint32_t>(pixels.size()),
                              texture, 0, info.width, info.height)) {
        SDL_ReleaseGPUTexture(device, texture);
        return nullptr;
    }
    return texture;
}

namespace {

constexpr int kMarker = 32;

float Side(float ax, float ay, float bx, float by, float px, float py) {
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

bool InTriangle(float px, float py, float ax, float ay, float bx, float by,
                float cx, float cy) {
    const float a = Side(ax, ay, bx, by, px, py);
    const float b = Side(bx, by, cx, cy, px, py);
    const float c = Side(cx, cy, ax, ay, px, py);
    return (a >= 0 && b >= 0 && c >= 0) || (a <= 0 && b <= 0 && c <= 0);
}

/// A chevron in the unit square, y down: tip at the top, a notch below.
bool InArrow(float x, float y) {
    return InTriangle(x, y, 0.5f, 0.06f, 0.9f, 0.92f, 0.1f, 0.92f) &&
           !InTriangle(x, y, 0.1f, 0.92f, 0.5f, 0.66f, 0.9f, 0.92f);
}

}  // namespace

bool CreateGta5MapMarkers(Gta5MapOverlay& map, SDL_GPUDevice* device,
                          Gta5UploadBatch& uploads) {
    std::vector<std::uint8_t> pixels(kMarker * kMarker * 4, 0);
    const float d = 1.5f / kMarker;  // the outline's reach
    for (int y = 0; y < kMarker; ++y) {
        for (int x = 0; x < kMarker; ++x) {
            const float fx = (x + 0.5f) / kMarker, fy = (y + 0.5f) / kMarker;
            const bool in = InArrow(fx, fy);
            const bool edge =
                !in && (InArrow(fx - d, fy) || InArrow(fx + d, fy) ||
                        InArrow(fx, fy - d) || InArrow(fx, fy + d));
            std::uint8_t* p = &pixels[(y * kMarker + x) * 4];
            if (in) {
                p[0] = 255, p[1] = 214, p[2] = 0, p[3] = 255;  // yellow
            } else if (edge) {
                p[3] = 255;  // black outline
            }
        }
    }
    map.marker = CreateGta5MapRgba(device, kMarker, kMarker, pixels, uploads);
    map.shade = CreateGta5MapRgba(device, 1, 1, {10, 14, 18, 235}, uploads);
    return map.marker && map.shade;
}

}  // namespace sdl3cpp::services::impl
