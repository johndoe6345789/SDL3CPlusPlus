#pragma once

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/// The single shared unit quad (and its upload transfer buffer) every
/// pickup billboard is drawn from. Owned by the step instance so it is
/// created once and released once, across many frames.
struct PickupQuadBuffers {
    SDL_GPUDevice* device           = nullptr;
    SDL_GPUBuffer* quadVb           = nullptr;
    SDL_GPUBuffer* quadIb           = nullptr;
    SDL_GPUTransferBuffer* transfer = nullptr;
};

/// Creates `buffers`' vertex/index buffers on first call; a no-op once
/// both are already created.
void EnsurePickupQuadBuffers(SDL_GPUDevice* device, PickupQuadBuffers& buffers);

/// Releases any GPU resources `buffers` owns. Safe to call unconditionally
/// (e.g. from a destructor) even if buffers were never created.
void ReleasePickupQuadBuffers(PickupQuadBuffers& buffers);

}  // namespace sdl3cpp::services::impl
