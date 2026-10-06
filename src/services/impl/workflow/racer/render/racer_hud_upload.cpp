#include "services/interfaces/workflow/racer/render/racer_hud_overlay.hpp"

#include "services/interfaces/workflow/rendering/gpu_overlay_quad.hpp"

#include <cstring>
#include <sstream>

namespace sdl3cpp::services::impl {
namespace {

constexpr Uint32 kPixelBytes = kRacerHudWidth * kRacerHudHeight * 4;

void DrawText(const RacerHudOverlay& hud, const std::string& text) {
    SDL_SetRenderDrawColor(hud.renderer, 0, 0, 0, 0);
    SDL_RenderClear(hud.renderer);
    SDL_SetRenderDrawColor(hud.renderer, 255, 220, 50, 255);
    std::istringstream lines(text);
    std::string line;
    for (float y = 2.f; std::getline(lines, line); y += 10.f) {
        SDL_RenderDebugText(hud.renderer, 4.f, y, line.c_str());
    }
    SDL_RenderPresent(hud.renderer);
}

}  // namespace

void UploadRacerHud(RacerHudOverlay& hud, SDL_GPUCommandBuffer* cmd,
                    const std::string& text, int frameWidth,
                    int frameHeight) {
    if (!cmd || !hud.renderer) return;
    DrawText(hud, text);
    auto* mapped = static_cast<std::uint8_t*>(
        SDL_MapGPUTransferBuffer(hud.device, hud.transfer, true));
    if (!mapped) return;
    std::memcpy(mapped, hud.surface->pixels, kPixelBytes);
    const GpuOverlayQuad quad = BuildGpuOverlayQuad(
        frameWidth, frameHeight, kRacerHudWidth * kRacerHudScale,
        kRacerHudHeight * kRacerHudScale, 24.f);
    std::memcpy(mapped + kPixelBytes, quad.data(), sizeof(quad));
    SDL_UnmapGPUTransferBuffer(hud.device, hud.transfer);
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    if (!copy) return;
    SDL_GPUTextureTransferInfo src = {};
    src.transfer_buffer = hud.transfer;
    src.pixels_per_row = kRacerHudWidth;
    src.rows_per_layer = kRacerHudHeight;
    SDL_GPUTextureRegion dst = {};
    dst.texture = hud.texture;
    dst.w = kRacerHudWidth;
    dst.h = kRacerHudHeight;
    dst.d = 1;
    SDL_UploadToGPUTexture(copy, &src, &dst, true);
    SDL_GPUTransferBufferLocation quadSrc = {hud.transfer, kPixelBytes};
    SDL_GPUBufferRegion quadDst = {hud.quad, 0, sizeof(quad)};
    SDL_UploadToGPUBuffer(copy, &quadSrc, &quadDst, true);
    SDL_EndGPUCopyPass(copy);
    hud.quadUploaded = true;
}

}  // namespace sdl3cpp::services::impl
