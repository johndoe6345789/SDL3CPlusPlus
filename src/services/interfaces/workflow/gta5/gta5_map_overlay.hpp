#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_upload_batch.hpp"

#include <SDL3/SDL_gpu.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// Backdrop, six tiles, marker.
inline constexpr int kGta5MapQuads = 8;

/// GTA's own pause-map art -- six 512 x 512 tiles, minimap_R_C.ytd, row R
/// counting south and column C east -- with a dimmed backdrop and a
/// marker, drawn over the frame by gta5.map.draw.
struct Gta5MapOverlay {
    SDL_GPUGraphicsPipeline* pipeline{nullptr};
    SDL_GPUBuffer* vertices{nullptr};
    SDL_GPUTransferBuffer* staging{nullptr};
    SDL_GPUSampler* sampler{nullptr};
    std::array<SDL_GPUTexture*, 6> tiles{};  // row * 2 + column
    SDL_GPUTexture* marker{nullptr};
    SDL_GPUTexture* shade{nullptr};
    bool tried{false};
    bool ready{false};
};

/// Create the overlay's pipeline and buffers and read the tiles from `dir`,
/// the extract's data/cdimages/scaleform_generic.rpf. Uploads go through
/// `uploads`, flushed here. False, and logged, when anything is missing.
bool LoadGta5MapOverlay(Gta5MapOverlay& map, SDL_GPUDevice* device,
                        SDL_GPUTextureFormat format, const std::string& dir,
                        Gta5UploadBatch& uploads,
                        const std::shared_ptr<ILogger>& logger);

/// The marker -- a yellow arrow pointing up -- and a one-texel backdrop.
bool CreateGta5MapMarkers(Gta5MapOverlay& map, SDL_GPUDevice* device,
                          Gta5UploadBatch& uploads);

/// Upload `quads` (see BuildGta5MapQuads) and draw the overlay over the
/// swapchain image, keeping what the frame already drew there.
void DrawGta5MapOverlay(Gta5MapOverlay& map, SDL_GPUDevice* device,
                        SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* swapchain,
                        const std::vector<float>& quads);

/// The overlay's quads in clip space, six vertices of xyz + uv each: the
/// backdrop over the screen, the tiles fitted to it at the map's 2 : 3,
/// and the marker at (u, v) in [0, 1] of the map, turned `angle` radians
/// clockwise from up.
std::vector<float> BuildGta5MapQuads(int width, int height, float u, float v,
                                     float angle);

}  // namespace sdl3cpp::services::impl
