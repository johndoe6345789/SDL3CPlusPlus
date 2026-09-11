#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_map_poi.hpp"
#include "services/interfaces/workflow/gta5/gta5_upload_batch.hpp"

#include <SDL3/SDL_gpu.h>

#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// Room in the overlay's vertex buffer, in quads of six vertices.
inline constexpr std::uint32_t kGta5MapMaxQuads = 1024;

/// GTA's own pause-map art -- six 512 x 512 tiles, minimap_R_C.ytd, row R
/// counting south and column C east -- under points of interest, a
/// legend and the player's arrow, drawn over the frame by gta5.map.draw.
struct Gta5MapOverlay {
    SDL_GPUGraphicsPipeline* pipeline{nullptr};
    SDL_GPUBuffer* vertices{nullptr};
    SDL_GPUTransferBuffer* staging{nullptr};
    SDL_GPUSampler* sampler{nullptr};  // linear: the art and the icons
    SDL_GPUSampler* nearest{nullptr};  // the lettering's square dots
    std::array<SDL_GPUTexture*, 6> tiles{};  // row * 2 + column
    SDL_GPUTexture* marker{nullptr};
    SDL_GPUTexture* shade{nullptr};
    SDL_GPUTexture* atlas{nullptr};  // category icons, then glyphs
    Gta5MapPois pois;
    bool tried{false};
    bool ready{false};
};

/// Create the pipeline, buffers and samplers, read the tiles from `dir`
/// (the extract's data/cdimages/scaleform_generic.rpf) and the points of
/// interest from `poiFile`, and build the icon atlas. Uploads go through
/// `uploads`, flushed here. False, and logged, when anything is missing.
bool LoadGta5MapOverlay(Gta5MapOverlay& map, SDL_GPUDevice* device,
                        SDL_GPUTextureFormat format, const std::string& dir,
                        const std::string& poiFile, Gta5UploadBatch& uploads,
                        const std::shared_ptr<ILogger>& logger);

/// Vertex and staging buffers for kGta5MapMaxQuads, and both samplers.
bool CreateGta5MapGpu(Gta5MapOverlay& map, SDL_GPUDevice* device);

}  // namespace sdl3cpp::services::impl
