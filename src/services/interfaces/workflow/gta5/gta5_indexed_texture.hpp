#pragma once

#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>

namespace sdl3cpp::services::impl {

/// A texture by name hash, from whichever .ytd the asset index says holds
/// it: uploaded on first use, shared from then on, and remembered as
/// missing when no dictionary has it so the search is not repeated.
const Gta5Texture* GetOrLoadGta5IndexedTexture(Gta5StreamState& state,
                                               std::uint32_t nameHash,
                                               SDL_GPUDevice* device);

}  // namespace sdl3cpp::services::impl
