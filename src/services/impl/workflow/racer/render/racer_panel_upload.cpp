#include "services/interfaces/workflow/racer/render/racer_panel.hpp"

#include <cstring>

namespace sdl3cpp::services::impl {
void UploadRacerPanelText(RacerPanel& panel, SDL_GPUCommandBuffer* cmd,
                          const std::vector<RacerPanelLine>& lines,
                          Uint8 shade) {
    if (!cmd || !panel.renderer) return;
    SDL_SetRenderDrawColor(panel.renderer, 0, 0, 0, shade);
    SDL_RenderClear(panel.renderer);
    for (const RacerPanelLine& line : lines) {
        const SDL_Color c = line.colour;
        SDL_SetRenderDrawColor(panel.renderer, c.r, c.g, c.b, c.a);
        if (line.boxWidth > 0.f) {
            const SDL_FRect box{line.x, line.y, line.boxWidth, line.boxHeight};
            SDL_RenderFillRect(panel.renderer, &box);
            continue;
        }
        SDL_RenderDebugText(panel.renderer, line.x, line.y, line.text.c_str());
    }
    SDL_RenderPresent(panel.renderer);
    void* mapped = SDL_MapGPUTransferBuffer(panel.device, panel.transfer, true);
    if (!mapped) return;
    std::memcpy(mapped, panel.surface->pixels,
                static_cast<std::size_t>(panel.width * panel.height * 4));
    SDL_UnmapGPUTransferBuffer(panel.device, panel.transfer);
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    if (!copy) return;
    SDL_GPUTextureTransferInfo src = {};
    src.transfer_buffer = panel.transfer;
    src.pixels_per_row = static_cast<Uint32>(panel.width);
    src.rows_per_layer = static_cast<Uint32>(panel.height);
    SDL_GPUTextureRegion dst = {};
    dst.texture = panel.texture;
    dst.w = static_cast<Uint32>(panel.width);
    dst.h = static_cast<Uint32>(panel.height);
    dst.d = 1;
    SDL_UploadToGPUTexture(copy, &src, &dst, true);
    SDL_EndGPUCopyPass(copy);
}

}  // namespace sdl3cpp::services::impl
